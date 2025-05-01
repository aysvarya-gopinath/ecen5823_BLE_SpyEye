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
#include"ble.h"
// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

// Buffer for receiving data
uint8_t read_lux[2];  // Holds the light sensor data
uint16_t light_data;   // Variable to store light level (lux)
static I2C_TransferSeq_TypeDef transferSequence;

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
  NVIC_EnableIRQ (I2C0_IRQn); // config NVIC to generate an IRQ for the I2C0 module
  transferStatus = I2C_TransferInit (I2C0, &transferSequence); //initialize the i2c transfer
    if (transferStatus != i2cTransferInProgress)
        LOG_ERROR("\n\r I2C write failed with %d status\n",  transferStatus);
    else
      LOG_INFO("i2c write in progress");
}

//Power up the sensor
void veml6030_powerON(void)
{
  //power on the sensor with power save mode for continuous measurement
  veml6030_write_register(VEML6030_REG_CONFIG, POWER_UP);
  LOG_INFO("ambient light sensor powering up");

}

//Set the power mode for the sensor
void veml6030_powerMode(void)
{
veml6030_write_register(VEML6030_REG_POWER,POWER_MODE);
LOG_INFO("setting ambient light sensor power mode");
}

// Initialize I2C for veml6030 sensor
void veml6030_init(void) {
  // Configure veml6030 for  .125 gain and 100ms integration time
  veml6030_write_register(VEML6030_REG_CONFIG,CONFIG_CODE);
  LOG_INFO("ambient light sensor initialized");
}


//Read the set configurations
void config_read(void)
{
  LOG_INFO("Reading the register configuration");
  uint8_t reg_address = VEML6030_REG_CONFIG;
  uint8_t config_data[2];
  I2C_TransferReturn_TypeDef transferStatus;
  LOG_INFO("i2c config read started");

    transferSequence.addr = VEML6030_I2C_ADDR << 1;
    transferSequence.flags = I2C_FLAG_WRITE_READ;
    transferSequence.buf[0].data = &reg_address;
    transferSequence.buf[0].len = 1;
    transferSequence.buf[1].data = config_data;
    transferSequence.buf[1].len = 2;
    NVIC_EnableIRQ (I2C0_IRQn); // config NVIC to generate an IRQ for the I2C0 module
    transferStatus = I2C_TransferInit (I2C0, &transferSequence); //initilaises the i2c transfer
    if (transferStatus != i2cTransferInProgress)
        LOG_ERROR("I2C read failed with status %d\n", transferStatus);
    else
      LOG_INFO("i2c read in progress");

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


//Read the light data
void veml6030_read_data(void) {
  I2C_TransferReturn_TypeDef transferStatus;
   uint8_t reg_address = VEML6030_REG_RESULT; // Register address for light result data
   LOG_INFO("i2c read started");

    transferSequence.addr = VEML6030_I2C_ADDR << 1;
    transferSequence.flags = I2C_FLAG_WRITE_READ;
    transferSequence.buf[0].data = &reg_address;
    transferSequence.buf[0].len = 1;
    transferSequence.buf[1].data = read_lux;
    transferSequence.buf[1].len = 2;
    NVIC_EnableIRQ (I2C0_IRQn); // config NVIC to generate an IRQ for the I2C0 module
    transferStatus = I2C_TransferInit (I2C0, &transferSequence);
    if (transferStatus != i2cTransferInProgress)//returns i2cTransferInProgress while performing the transfer
        LOG_ERROR("I2C read failed with status %d\n", transferStatus);
    else
      LOG_INFO("i2c read in progress");
}


//convert the raw data to value
void veml6030_conversion(void)
{
 // const char *alert_message = "Found";

  // Convert the 16-bit raw data to lux
        light_data = (uint16_t)read_lux[0]  | ( (uint16_t)read_lux[1]<< 8); //lsb first
        float lux = 0.0576* light_data;
        LOG_INFO("Uncorrected Lux: %.2f lux\n", lux);

            // Apply non-linearity correction if lux > 1000
            if (lux > 1000.0) {
                float corrected_lux = (6.0135e-13 * pow(lux, 4)) -
                                      (9.3924e-9  * pow(lux, 3)) +
                                      (8.1488e-5  * pow(lux, 2)) +
                                      (1.0023     * lux);
                printf("Corrected  Light level Lux: %.2f lx\n", corrected_lux);

            }
           // else
            if(lux<300)
              { send_temp_ble(1);
            LOG_INFO("INTRUDER FOUND");
              }
            else
              {send_temp_ble(0);
              LOG_INFO("SAFE");
              }
}






