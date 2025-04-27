/*
 * veml6030.c
 *
 *  Created on: Apr 18, 2025
 *      Author: aysva
 *      References:https://os.mbed.com/teams/MSS/code/VEML6030//file/00f62b381f9e/VEML6030.cpp/
 */
#include "veml6030.h"
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

/*void i2c_scan_bus(void) {
    I2C_TransferSeq_TypeDef seq;
    I2C_TransferReturn_TypeDef result;
    uint8_t dummy_data = 0;

    LOG_INFO("Scanning I2C bus...\n");

    for (uint8_t address = 0x03; address <= 0x77; address++) {
        seq.addr = address << 1; // Left-shift for 8-bit address format
        seq.flags = I2C_FLAG_WRITE;
        seq.buf[0].data = &dummy_data;
        seq.buf[0].len = 1;

        result = I2CSPM_Transfer(I2C0, &seq);
        //LOG_INFO("Checking if 0x%02X\n is the device", address);
        if (result == i2cTransferDone) {
            LOG_INFO("Yes Device found at 0x%02X\n", address);
        }
        else
          LOG_INFO("Not the device 0x%02X\n", address);
    }

    LOG_INFO("I2C scan complete.\n");
}
*/

/* Write a 16-bit value to an veml6030 register */
void veml6030_write_register(uint8_t reg, uint16_t value) {
  I2C_TransferReturn_TypeDef transferStatus;
  uint8_t tx_data[3];
  LOG_INFO("i2c write started");
  tx_data[0] = reg;
  tx_data[1] = value & 0xFF;
  tx_data[2] = (value >> 8) & 0xFF;

  transferSequence.addr = VEML6030_I2C_ADDR << 1;// veml6030 address
  transferSequence.flags = I2C_FLAG_WRITE;// Write flag
  transferSequence.buf[0].data = tx_data;
  transferSequence.buf[0].len = 3;
  transferStatus = I2CSPM_Transfer ( I2C0, &transferSequence); //blocking call
    if (transferStatus != i2cTransferDone)
        LOG_ERROR("\n\r I2C write failed with %d status\n",  transferStatus);
    else
      LOG_INFO("i2c write complete");
}

//Power up the sensor
void veml6030_powerON(void)
{
  //power on the sensor with power save mode for continuous measurement
  veml6030_write_register(VEML6030_REG_CONFIG, POWER_UP);
  veml6030_write_register(VEML6030_REG_POWER,POWER_MODE);
  LOG_INFO("ambient light sensor powering up");

}

/* Initialize I2C for veml6030 sensor */
void veml6030_init(void) {
  // Configure veml6030 for  .125 gain and 100ms integration time
  veml6030_write_register(VEML6030_REG_CONFIG,CONFIG_CODE);
  LOG_INFO("ambient light sensor initialized");
}


void config_read(void)
{

  LOG_INFO("Reading the register configuration");
  uint8_t reg_address = VEML6030_REG_CONFIG;
  uint8_t config_data[2];
  I2C_TransferReturn_TypeDef transferStatus;
  LOG_INFO("i2c config read started");
  // Write the register address
 transferSequence.addr = VEML6030_I2C_ADDR << 1;
  transferSequence.flags = I2C_FLAG_WRITE;
  transferSequence.buf[0].data = &reg_address;
  transferSequence.buf[0].len = 1;

  transferStatus = I2CSPM_Transfer(I2C0, &transferSequence);
  if (transferStatus != i2cTransferDone)
      LOG_ERROR("I2C write failed with status %d\n", transferStatus);

  // Read 2 bytes from the configuration register
  transferSequence.addr = VEML6030_I2C_ADDR << 1;
  transferSequence.flags = I2C_FLAG_READ;
  transferSequence.buf[0].data = config_data;
  transferSequence.buf[0].len = 2;

  transferStatus = I2CSPM_Transfer(I2C0, &transferSequence);
  if (transferStatus != i2cTransferDone)
      LOG_ERROR("I2C read failed with status %d\n", transferStatus);



  /*
    transferSequence.addr = VEML6030_I2C_ADDR << 1;
    transferSequence.flags = I2C_FLAG_WRITE_READ;
    transferSequence.buf[0].data = &reg_address;
    transferSequence.buf[0].len = 1;
    transferSequence.buf[1].data = config_data;
    transferSequence.buf[1].len = 2;
    transferStatus = I2CSPM_Transfer(I2C0, &transferSequence);
    if (transferStatus != i2cTransferDone)
        LOG_ERROR("I2C read failed with status %d\n", transferStatus);
*/
  // Combine LSB and MSB
  uint16_t config_value = config_data[0] | (config_data[1] << 8);
  uint8_t gain_bits = (config_value >> 11) & 0x03;
     uint8_t it_bits = (config_value >> 6) & 0x0F;

     // Interpret gain
     const char* gain_str;
     switch (gain_bits) {
         case 0x00: gain_str = "Gain x1"; break;
         case 0x01: gain_str = "Gain x2"; break;
         case 0x02: gain_str = "Gain x1/8"; break;
         case 0x03: gain_str = "Gain x1/4"; break;
         default: gain_str = "Unknown Gain"; break;
     }

     // Interpret integration time
     const char* it_str;
     switch (it_bits) {
         case 0x0C: it_str = "25 ms"; break;
         case 0x08: it_str = "50 ms"; break;
         case 0x00: it_str = "100 ms"; break;
         case 0x01: it_str = "200 ms"; break;
         case 0x02: it_str = "400 ms"; break;
         case 0x03: it_str = "800 ms"; break;
         default: it_str = "Unknown IT"; break;
     }

     LOG_INFO("Gain setting: %s\n", gain_str);
     LOG_INFO("Integration time: %s\n", it_str);

}



/*void veml6030_read_data(void) {
  I2C_TransferReturn_TypeDef transferStatus;
   uint8_t reg_address = VEML6030_REG_RESULT; // Register address for light result data
   LOG_INFO("i2c read started");

    transferSequence.addr = VEML6030_I2C_ADDR << 1;
    transferSequence.flags = I2C_FLAG_WRITE_READ;
    transferSequence.buf[0].data = &reg_address;
    transferSequence.buf[0].len = 1;
    transferSequence.buf[1].data = read_lux;
    transferSequence.buf[1].len = 2;
    transferStatus = I2CSPM_Transfer(I2C0, &transferSequence);
    if (transferStatus != i2cTransferDone)
        LOG_ERROR("I2C read failed with status %d\n", transferStatus);
}

*/

// Read data (light level) from the veml6030 sensor
void veml6030_read_data(void) {
  I2C_TransferReturn_TypeDef transferStatus;
  uint8_t reg_address = VEML6030_REG_RESULT; // Register address for light result data
  LOG_INFO("i2c read started");
  //  Write the register address (only 1 byte)
  transferSequence.addr = VEML6030_I2C_ADDR << 1;  // Address for write
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
   transferSequence.addr = VEML6030_I2C_ADDR << 1;  // Address for read
  transferSequence.flags = I2C_FLAG_READ; // Change to read
  transferSequence.buf[0].data = read_lux; // Buffer for the read data
  transferSequence.buf[0].len = 2; // Length of the data to read
  // Perform the I2C read transfer
  transferStatus = I2CSPM_Transfer ( I2C0, &transferSequence);
  if (transferStatus != i2cTransferDone)
      LOG_ERROR("\n\r I2C read failed with %d status\n",  transferStatus);
  else
      LOG_INFO("i2c read complete");

 }


//convert the raw data to value
void veml6030_conversion(void)
{
  // Convert the 16-bit raw data to lux
        light_data = (uint16_t)read_lux[0]  | ( (uint16_t)read_lux[1]<< 8); //lsb first
        LOG_INFO("Raw ALS value: %u\n", light_data);
        float lux =  0.4608 * light_data;
        LOG_INFO("Uncorrected Lux: %.2f lux\n", lux);

            // Apply non-linearity correction if lux > 1000
            if (lux > 1000.0) {
                float corrected_lux = (6.0135e-13 * pow(lux, 4)) -
                                      (9.3924e-9  * pow(lux, 3)) +
                                      (8.1488e-5  * pow(lux, 2)) +
                                      (1.0023     * lux);
                printf("Corrected  Light level Lux: %.2f lx\n", corrected_lux);
            }
}

/* I2C interrupt handler */
/*void veml6030_I2C_IRQHandler(void) {
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
 //   schedulerSetEvent_veml6030Read();
  } else if (transferStatus < 0) {
    LOG_ERROR("I2C transfer failed with error code: %d\n", transferStatus);
  }
}

*/



