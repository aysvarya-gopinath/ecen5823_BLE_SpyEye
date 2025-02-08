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
#define TEMP_OFFSET (46.85) //Temperature offset of sensor
#define SCALING_FACTOR (175.72)

//initialize i2c
void i2c_init(void);

// performs i2c read
I2C_TransferReturn_TypeDef i2c_read( uint8_t *read_cmd, uint16_t read_cmd_len);

//performs i2c write
I2C_TransferReturn_TypeDef i2c_write( uint8_t *write_cmd, uint16_t write_cmd_len);

//function to read the temperature from the sensor
void  read_temp_from_si7021(double *temperature);

//function to write command to the sensor
void  write_cmd_to_si7021(void);


#endif /* SRC_I2C_H_ */
