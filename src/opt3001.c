/*
 * opt3001.c
 *
 *  Created on: Apr 18, 2025
 *      Author: aysva
 */
#include "opt3001.h"


#include "opt3001.h"
#include "em_i2c.h"
#include "em_gpio.h"
#include "em_cmu.h"
#include "log.h"

// Buffer for receiving data
uint8_t read_data[2];  // Holds the light sensor data
uint16_t light_data;   // Variable to store light level (lux)

static I2C_TransferSeq_TypeDef transferSequence;

/* Initialize I2C for OPT3001 sensor */
void opt3001_init(void) {
  static I2CSPM_Init_TypeDef I2C_Config = {
    .port            = I2C0,
    .sclPort         = gpioPortC,
    .sclPin          = SCL_pin,
    .sdaPort         = gpioPortC,
    .sdaPin          = SDA_pin,
    .portLocationScl = 14,
    .portLocationSda = 16,
    .i2cRefFreq      = 0,
    .i2cMaxFreq      = I2C_FREQ_STANDARD_MAX,
    .i2cClhr         = i2cClockHLRStandard
  };
  I2CSPM_Init(&I2C_Config);

  // Send initialization command to OPT3001 to configure the sensor
  opt3001_send_command(OPT3001_CONFIG_DEFAULT);
}

/* Send command to the OPT3001 sensor */
void opt3001_send_command(uint8_t command) {
  I2C_TransferReturn_TypeDef transferStatus;

  transferSequence.addr = OPT3001_I2C_ADDR << 1; // OPT3001 address
  transferSequence.flags = I2C_FLAG_WRITE;       // Write flag
  transferSequence.buf[0].data = &command;       // Command data
  transferSequence.buf[0].len = sizeof(command);

  NVIC_EnableIRQ(I2C0_IRQn); // Enable I2C interrupt
  transferStatus = I2C_TransferInit(I2C0, &transferSequence);

  if (transferStatus != i2cTransferInProgress) {
    LOG_ERROR("\n\rI2C bus command write failed with status: %d\n", transferStatus);
  }
}

/* Read data (light level) from the OPT3001 sensor */
void opt3001_read_data(void) {
  I2C_TransferReturn_TypeDef transferStatus;

  // Reading the result register to get light data
  transferSequence.addr = OPT3001_I2C_ADDR << 1; // OPT3001 address
  transferSequence.flags = I2C_FLAG_READ;        // Read flag
  transferSequence.buf[0].data = read_data;      // Data buffer to store result
  transferSequence.buf[0].len = sizeof(read_data);

  NVIC_EnableIRQ(I2C0_IRQn); // Enable I2C interrupt
  transferStatus = I2C_TransferInit(I2C0, &transferSequence);

  if (transferStatus != i2cTransferInProgress) {
    LOG_ERROR("\n\rI2C bus temperature read failed with status: %d\n", transferStatus);
  }
}

/* I2C interrupt handler for OPT3001 */
void opt3001_I2C_IRQHandler(void) {
  I2C_TransferReturn_TypeDef transferStatus;

  transferStatus = I2C_Transfer(I2C0);
  if (transferStatus == i2cTransferDone) {
    NVIC_DisableIRQ(I2C0_IRQn); // Disable interrupt after transfer is complete

    // Convert the 16-bit data received into a lux value
    light_data = (read_data[0] << 8) | read_data[1];  // Combine high and low byte
    // Log or process the light data as needed
    LOG_INFO("Light level: %d lux\n", light_data);

    // Scheduler can set an event here if using an RTOS (not shown)
    schedulerSetEvent_opt3001Read(); // Example: notify when reading is complete
  } else if (transferStatus < 0) {
    LOG_ERROR("I2C transfer failed with error code: %d", transferStatus);
  }
}


