/*
 * i2c.h
 *This file contains the declaration of functions for i2c communication
 * Created on: Feb 3, 2025
   Author:@Aysvarya Gopinath   Aysvarya.Gopinath@Colorado.edu
 *
 */
#ifndef SRC_I2C_H_
#define SRC_I2C_H_

#include"em_i2c.h"

#define SCL_pin    (10) //PC10
#define SDA_pin     (11)//PC11
#define SI7021_DEVICE_ADDR  (0x40)// slave address 0x40
#define MEASURE_TEMP_NO_HOLD_MASTER (0XF3)//Measure Temperature, No Hold Master Mode 0xF3
#define MAX_16bit (65536)
typedef struct
{
        uint8_t  data[ 2 ];
        uint16_t len;
} I2C_Data;



void i2c_init(void);
I2C_TransferReturn_TypeDef i2c_read( uint8_t *read_cmd, uint16_t read_cmd_len);

I2C_TransferReturn_TypeDef i2c_write( uint8_t *write_cmd, uint16_t write_cmd_len);

void  read_temp_from_si7021(double *temperature);



#endif /* SRC_I2C_H_ */
