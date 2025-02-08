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
// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

uint8_t  read_data[ 2 ]; //to hold sensor data

// Initialize the I2C hardware
static I2CSPM_Init_TypeDef  I2C_Config = {
        .port            = I2C0,
        .sclPort         = gpioPortC,
        .sclPin          =  SDA_pin ,
        .sdaPort         = gpioPortC,
        .sdaPin          =  SDA_pin ,
        .portLocationScl = 14,
        .portLocationSda = 16,
        .i2cRefFreq      = 0,
        .i2cMaxFreq      = I2C_FREQ_STANDARD_MAX,
        .i2cClhr         = i2cClockHLRStandard
};

//initialization function
void i2c_init(){
I2CSPM_Init(& I2C_Config);
}

//read function
I2C_TransferReturn_TypeDef i2c_read( uint8_t *read_data, uint16_t read_data_len)
{
  I2C_TransferSeq_TypeDef transferSequence;
  I2C_TransferReturn_TypeDef transferStatus;
  transferSequence.addr = SI7021_DEVICE_ADDR << 1;
  transferSequence.flags = I2C_FLAG_READ;
  transferSequence.buf[0].data = read_data;
  transferSequence.buf[0].len = read_data_len;
  transferStatus = I2CSPM_Transfer ( I2C0, &transferSequence);
  return transferStatus;
}

//write function
I2C_TransferReturn_TypeDef i2c_write( uint8_t *write_cmd, uint16_t write_cmd_len)
{
  I2C_TransferSeq_TypeDef transferSequence;
  I2C_TransferReturn_TypeDef transferStatus;
  transferSequence.addr = SI7021_DEVICE_ADDR << 1;
  transferSequence.flags = I2C_FLAG_WRITE;
  transferSequence.buf[0].data = write_cmd;
  transferSequence.buf[0].len = write_cmd_len;
  transferStatus = I2CSPM_Transfer ( I2C0, &transferSequence);
  return transferStatus;
}

//write command via i2c
void  write_cmd_to_si7021(void)
{
  I2C_TransferReturn_TypeDef status;
  uint8_t write_cmd = MEASURE_TEMP_NO_HOLD_MASTER; //temperature read command
  uint16_t write_cmd_len = sizeof(write_cmd); //length of buff data
  status = i2c_write (&write_cmd, write_cmd_len); //write the command
  if (status != i2cTransferDone)
    {
      LOG_ERROR("\n\r I2C bus command write failed with %d status\n", status);
    }
}

void  read_temp_from_si7021(double *temperature)
{
  I2C_TransferReturn_TypeDef status;
  uint16_t read_data_len = sizeof(read_data); //length of the argument to hold the data
  status = i2c_read (read_data, read_data_len); //read the temperature
  if (status == i2cTransferDone)
    {
      uint32_t temp_raw = ((uint32_t) read_data[0] << 8)
          | (uint32_t) read_data[1]; //extract temperature from raw data
      *temperature = ((SCALING_FACTOR * temp_raw) / MAX_16bit) - TEMP_OFFSET; //convert to celcius
      LOG_INFO("\n\r Temperature: %f C\n", *temperature);
    }
  else
    {
      LOG_ERROR("\n\r I2C bus temperature read failed with %d status\n",
                status);
    }

}




