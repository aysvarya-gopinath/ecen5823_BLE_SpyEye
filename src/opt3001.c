/*
 * opt3001.c
 *
 *  Created on: Apr 18, 2025
 *      Author: aysva
 */
#include "opt3001.h"
#include "em_i2c.h"
#include "em_gpio.h"
#include "em_cmu.h"
#include "sl_i2cspm.h"
#include"gpio.h"
#include"em_i2c.h"
#include"stdint.h"
#include"em_cmu.h"

// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

// Buffer for receiving data
uint8_t read_lux[2];  // Holds the light sensor data
uint16_t light_data;   // Variable to store light level (lux)
static I2C_TransferSeq_TypeDef transferSequence;
uint16_t config = 0xCC00 ;//110011000000000; // Continuous conversions, 800ms, automatic full scale


/* Write a 16-bit value to an OPT3001 register */
void opt3001_write_register(uint8_t reg, uint16_t value) {
  I2C_TransferReturn_TypeDef transferStatus;
  uint8_t tx_data[3];
  LOG_INFO("i2c write started");
  tx_data[0] = reg;
  tx_data[1] = (value >> 8) & 0xFF;
  tx_data[2] = value & 0xFF;

  transferSequence.addr = OPT3001_I2C_ADDR << 1;// OPT3001 address
  transferSequence.flags = I2C_FLAG_WRITE;// Write flag
  transferSequence.buf[0].data = tx_data;
  transferSequence.buf[0].len = 3;
  transferStatus = I2CSPM_Transfer ( I2C0, &transferSequence); //blocking call
    if (transferStatus != i2cTransferDone)
        LOG_ERROR("\n\r I2C write failed with %d status\n",  transferStatus);
    else
      LOG_INFO("i2c write complete");

}


/* Initialize I2C for OPT3001 sensor */
void opt3001_init(void) {
  // Configure OPT3001 to continuous conversion mode
  opt3001_write_register(OPT3001_REG_CONFIG, config);
  LOG_INFO("ambient light sensor initialized");
}


/* Read data (light level) from the OPT3001 sensor */
void opt3001_read_data(void) {
  I2C_TransferReturn_TypeDef transferStatus;
  uint8_t reg_address = OPT3001_REG_RESULT; // Register address for light result data
  LOG_INFO("i2c read started");
  //  Write the register address (only 1 byte)
  transferSequence.addr = OPT3001_I2C_ADDR << 1;  // Address for write
  transferSequence.flags = I2C_FLAG_WRITE;
  transferSequence.buf[0].data = &reg_address;
  transferSequence.buf[0].len = 1; // Only 1 byte for register address

  // Perform the I2C transfer for register address
  transferStatus = I2CSPM_Transfer ( I2C0, &transferSequence);
   if (transferStatus != i2cTransferDone)
       LOG_ERROR("\n\r I2C parial write failed with %d status\n",  transferStatus);
   else
       LOG_INFO("i2c partial write complete");

  //  Read the data (light level) from the register
  transferSequence.flags = I2C_FLAG_READ; // Change to read
  transferSequence.buf[0].data = read_lux; // Buffer for the read data
  transferSequence.buf[0].len = sizeof(read_lux); // Length of the data to read

  // Perform the I2C read transfer
  transferStatus = I2CSPM_Transfer ( I2C0, &transferSequence);
  if (transferStatus != i2cTransferDone)
      LOG_ERROR("\n\r I2C read failed with %d status\n",  transferStatus);
  else
      LOG_INFO("i2c read complete");

 }



//convert the raw data to value
void opt3001_conversion(void)
{
  // Convert the 16-bit raw data to lux
        light_data = (read_lux[0] << 8) | read_lux[1];

        uint16_t exponent = (light_data >> 12) & 0x0F;
        uint16_t mantissa = light_data & 0x0FFF;
        float lux = 0.01 * (1 << exponent) * mantissa;

        LOG_INFO("Light level: %.2f lux\n", lux);
}
/* I2C interrupt handler */
/*void opt3001_I2C_IRQHandler(void) {
  I2C_TransferReturn_TypeDef transferStatus;

  transferStatus = I2C_Transfer(I2C0);

  if (transferStatus == i2cTransferDone) {
    NVIC_DisableIRQ(I2C0_IRQn); // Disable interrupt after transfer is complete

    // Convert the 16-bit raw data to lux
    light_data = (read_lux[0] << 8) | read_lux[1];

    uint16_t exponent = (light_data >> 12) & 0x0F;
    uint16_t mantissa = light_data & 0x0FFF;
    float lux = 0.01 * (1 << exponent) * mantissa;

    LOG_INFO("Light level: %.2f lux\n", lux);

    // Notify scheduler or application
 //   schedulerSetEvent_opt3001Read();
  } else if (transferStatus < 0) {
    LOG_ERROR("I2C transfer failed with error code: %d\n", transferStatus);
  }
}

*/



