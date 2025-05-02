/*
 * i2c.c
 *This file contains the functions of i2c communication
 * Created on: Feb 3, 2025
   Author:@Aysvarya Gopinath   Aysvarya.Gopinath@Colorado.edu
 *
 */


#include "i2c.h"
#include "sl_i2cspm.h"
#include"gpio.h"
#include"em_i2c.h"
#include"stdint.h"
#include"em_cmu.h"
#include"scheduler.h"
#include"ble.h"

#include "lcd.h"
// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 0
#include "src/log.h"

uint8_t  read_data[ 2 ]; //to hold sensor data
double *temperature; //holds the temperature
uint8_t write_cmd = MEASURE_TEMP_NO_HOLD_MASTER;  //command to read data
static I2C_TransferSeq_TypeDef transferSequence;


/*Initialize the I2C hardware
 * No return type and parameters
 */
void i2c_init ()
{
  static I2CSPM_Init_TypeDef I2C_Config =
    {
          .port            = I2C0,
          .sclPort         = gpioPortC,
          .sclPin          =  SCL_pin ,
          .sdaPort         = gpioPortC,
          .sdaPin          =  SDA_pin ,
          .portLocationScl = 14,
          .portLocationSda = 16,
          .i2cRefFreq      = 0,
          .i2cMaxFreq      = I2C_FREQ_STANDARD_MAX,
          .i2cClhr         = i2cClockHLRStandard
  };
I2CSPM_Init(& I2C_Config);
}


/*I2C interrupt handler comes in action when the transfer is initialized
 * No return type and parameters
 */
void I2C0_IRQHandler(void) {
  I2C_TransferReturn_TypeDef transferStatus;
  transferStatus = I2C_Transfer (I2C0);
  if (transferStatus == i2cTransferDone) //returns i2cTransferDone when transfer is success
    {
      NVIC_DisableIRQ (I2C0_IRQn); //disable the interrupt when transfer is done
      schedulerSetEvent_i2cTransfer (); //call the event handler to set the i2c complete status
    }
  if (transferStatus <0)
    {
      LOG_ERROR("%d", transferStatus);
    }
}


