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
#include "lcd.h"
// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 0
#include "src/log.h"

// Buffer for receiving data
uint8_t read_lux[2];  // Holds the light sensor data
uint16_t light_data;   // Variable to store light level (lux)
static I2C_TransferSeq_TypeDef transferSequence;


/*This function writes a 16-bit value to an veml6030 register
* No parameters and return type
*/
void veml6030_write_register(uint8_t reg, uint16_t value) {
  I2C_TransferReturn_TypeDef transferStatus;
  uint8_t tx_data[3];
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
}

/*This function powers up the sensor
* No parameters and return type
*/
void veml6030_powerON(void)
{
  //power on the sensor by clearing the bit 0
  veml6030_write_register(VEML6030_REG_CONFIG, POWER_UP);
}

/*This function powers off the sensor
* No parameters and return type
*/
void veml6030_powerOFF(void)
{
  //power off the sensor
  veml6030_write_register(VEML6030_REG_CONFIG, POWER_OFF);
}


/*This function set the power mode for the sensor
* No parameters and return type
*/
void veml6030_powerMode(void)
{
veml6030_write_register(VEML6030_REG_POWER,POWER_MODE);
}


/*This function Initialize I2C for veml6030 sensor
* No parameters and return type
*/
void veml6030_init(void) {
  // Configure veml6030 for  1x gain and 100ms integration time with interrupt enabled
  veml6030_write_register(VEML6030_REG_CONFIG,CONFIG_CODE);
}


/*This function set the high threshold to  trigger interrupt
* No parameters and return type
*/
void veml6030_high_threshold(void)
{
  veml6030_write_register(VEML6030_REG_HIGH,HIGH_THRESHOLD);
}


/*This function set the low threshold to trigger an interrupt
* No parameters and return type
*/
void veml6030_low_threshold(void)
{
  veml6030_write_register(VEML6030_REG_LOW ,LOW_THRESHOLD);
}


/*This function reads the set configurations for the interrrupt
* No parameters and return type
*/
void config_read(void)
{
  uint8_t reg_address = VEML6030_REG_INTERRUPT ; //read if interrupt flags are set
  uint8_t config_data[2];
  I2C_TransferReturn_TypeDef transferStatus;

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

  // Combine LSB and MSB
  uint16_t config_value = config_data[0] | (config_data[1] << 8);
  uint8_t low_bits = (config_value >> 15) &  0x01;
     uint8_t high_bits = (config_value >> 14) &  0x01;
     if((low_bits)||(high_bits))
       {
         //LOG_INFO("INTERRUPT FLAGS ARE SET");
       }
}


/*This function reads the light data
* No parameters and return type
*/
void veml6030_read_data(void) {
  I2C_TransferReturn_TypeDef transferStatus;
   uint8_t reg_address = VEML6030_REG_RESULT; // Register address for light result data
    transferSequence.addr = VEML6030_I2C_ADDR << 1;
    transferSequence.flags = I2C_FLAG_WRITE_READ; //partial write of the address and repeated start to read data
    transferSequence.buf[0].data = &reg_address;
    transferSequence.buf[0].len = 1;
    transferSequence.buf[1].data = read_lux;
    transferSequence.buf[1].len = 2;
    NVIC_EnableIRQ (I2C0_IRQn); // config NVIC to generate an IRQ for the I2C0 module
    transferStatus = I2C_TransferInit (I2C0, &transferSequence);
    if (transferStatus != i2cTransferInProgress)//returns i2cTransferInProgress while performing the transfer
        LOG_ERROR("I2C read failed with status %d\n", transferStatus);
}


/*This function converts the raw data to value
* No parameters and return type
*/
void veml6030_conversion(void)
{
  // Convert the 16-bit raw data to lux
        light_data = (uint16_t)read_lux[0]  | ( (uint16_t)read_lux[1]<< 8); //lsb first
        float lux = 0.0576* light_data;
          //LOG_INFO("Uncorrected Lux: %.2f lux\n", lux);

            // Apply non-linearity correction if lux > 1000
            if (lux > 1000.0) {
                float corrected_lux = (6.0135e-13 * pow(lux, 4)) -
                                      (9.3924e-9  * pow(lux, 3)) +
                                      (8.1488e-5  * pow(lux, 2)) +
                                      (1.0023     * lux);
              //  printf("Corrected  Light level Lux: %.2f lx\n", corrected_lux);
            }

            if(lux<300)
              { send_motion_ble(1);
              displayPrintf (DISPLAY_ROW_TEMPVALUE, "INTRUDER FOUND");
              LOG_INFO("INTRUDER FOUND");//log info on terminal
              }
            else
              {
               send_motion_ble(0);
              displayPrintf (DISPLAY_ROW_TEMPVALUE, "SAFE");//display on server
              LOG_INFO("SAFE");
              }
}






