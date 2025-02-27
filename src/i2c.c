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
#define INCLUDE_LOG_DEBUG 1
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

/*I2C write function to send commands to the sensor
 * No return type and parameters
 */
void send_I2C_command (void)

{
  I2C_TransferReturn_TypeDef transferStatus;
  transferSequence.addr = SI7021_DEVICE_ADDR << 1; //device address
  transferSequence.flags = I2C_FLAG_WRITE; //write flag is set
  transferSequence.buf[0].data = &write_cmd; //holds the command to send
  transferSequence.buf[0].len = sizeof(write_cmd);
  NVIC_EnableIRQ (I2C0_IRQn); // config NVIC to generate an IRQ for the I2C0 module.
  transferStatus = I2C_TransferInit (I2C0, &transferSequence); //initialize the i2c transfer
  if (transferStatus != i2cTransferInProgress) //returns i2cTransferInProgress while performing the transfer
    {
      LOG_ERROR("\n\r I2C bus command write failed with %d status\n",transferStatus); //if not returns error
    }
}


/*I2C read function to recieve data from the sensor
 * No return type and parameters
 */
void read_temp_data()
{
  I2C_TransferReturn_TypeDef transferStatus;
  transferSequence.addr = SI7021_DEVICE_ADDR << 1; //device address
  transferSequence.flags = I2C_FLAG_READ;   //read flag is set
  transferSequence.buf[0].data = read_data;  //receives the read data
  transferSequence.buf[0].len = sizeof(read_data);
  NVIC_EnableIRQ (I2C0_IRQn); // config NVIC to generate an IRQ for the I2C0 module
  transferStatus = I2C_TransferInit (I2C0, &transferSequence); //initilaises the i2c transfer
  if (transferStatus != i2cTransferInProgress)//returns i2cTransferInProgress while performing the transfer
    {
      LOG_ERROR("\n\r I2C bus temperature read failed with %d status\n",transferStatus);//if not returns error
    }
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

/*Converts and logs the temperature in Celcius
 * No return type and parameters
 */
void log_temperature(){
  uint32_t temp_raw = ((uint32_t) read_data[0] << 8) | read_data[1]; //extract temperature from raw data
  int32_t temp_deg = (int32_t)((SCALING_FACTOR * temp_raw) / MAX_16bit) - TEMP_OFFSET; //convert to celcius
  //LOG_INFO("\n\r Temperature: %d C\n", temp_deg);
  displayPrintf(DISPLAY_ROW_TEMPVALUE,"Temperature=%u",temp_deg);
  send_temp_ble(temp_deg);  //send the temperature to the ble stack
}

