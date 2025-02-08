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

I2C_Data Buff;

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
I2C_TransferReturn_TypeDef i2c_read( uint8_t *read_cmd, uint16_t read_cmd_len)
{
    I2C_TransferSeq_TypeDef transferSequence;
    I2C_TransferReturn_TypeDef transferStatus;

    transferSequence.addr = SI7021_DEVICE_ADDR << 1;
    transferSequence.flags = I2C_FLAG_READ;
    transferSequence.buf[ 0 ].data = read_cmd;
    transferSequence.buf[ 0 ].len = read_cmd_len;
    transferStatus = I2CSPM_Transfer( I2C0, &transferSequence );
    if (transferStatus != i2cTransferDone){
    LOG_ERROR("\n\r I2C bus write of cmd failed");
    }
    return transferStatus;
}

//write function
I2C_TransferReturn_TypeDef i2c_write( uint8_t *write_cmd, uint16_t write_cmd_len)
{
    I2C_TransferSeq_TypeDef transferSequence;
    I2C_TransferReturn_TypeDef transferStatus;
    transferSequence.addr = SI7021_DEVICE_ADDR << 1;
    transferSequence.flags = I2C_FLAG_WRITE;
    transferSequence.buf[ 0 ].data = write_cmd;
    transferSequence.buf[ 0 ].len = write_cmd_len;
    transferStatus = I2CSPM_Transfer( I2C0, &transferSequence );
    if (transferStatus != i2cTransferDone){
       LOG_ERROR("\n\r I2C bus write of cmd failed");
       }
    return transferStatus;
}

void  write_cmd_to_si7021(void)
{
  I2C_TransferReturn_TypeDef status;
    uint8_t tries = 10;
    i2c_init(); //initialise the i2c
//    gpioSi7021ON();// Enable the sensor
//    // Wait for 80ms
//    timerWaitUs(80000);
    Buff.data[0] = MEASURE_TEMP_NO_HOLD_MASTER;//temperature read command
    Buff.len = 1; //length of buff data
    while (tries--)
    {
        status = i2c_write(Buff.data, Buff.len);
        if (status == i2cTransferDone)
        {
           // LOG_INFO("Tranfer Done!\n");
            break;
        }
        else
        {
            timerWaitUs(10000);
        }
    }
}
void  read_temp_from_si7021(double *temperature)
{I2C_TransferReturn_TypeDef status;
    //timerWaitUs(10000); // Wait for 10ms
  uint8_t  tries = 10;
    Buff.len = 2;
    while (tries--)
    {
        status = i2c_read(Buff.data, Buff.len);
        if (status == i2cTransferDone)
        {
            // Calculate the temperature value from the read data
            *temperature = ((uint32_t)Buff.data[0] << 8) | (Buff.data[1] & 0xFC);
            *temperature = ((175.72 * (*temperature)) / MAX_16bit) - 46.85;
            LOG_INFO("\n\r Temperature: %f\n", *temperature);
            break;
           // 175.72 → Scaling factor from sensor datasheet.
           // -46.85 → Temperature offset from sensor datasheet.
        }
        else
        {
            timerWaitUs(10000);//wait for 10000
        }

       }

       gpioSi7021OFF();// turn off the sensor
       I2C_Reset( I2C0 );// reset i2c
       I2C_Enable( I2C0, false );
       gpioI2CSDADisable();// disable sda
       gpioI2CSCLDisable();//disable scl
       CMU_ClockEnable( cmuClock_I2C0, false ); //turn off the clock
       return;
        }



