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

//write command via i2c
void send_I2C_command (void);
//read temperature via i2c
void read_temp_data(void);

//log the read temperature values
void log_temperature(void);

//i2c interrupt handler
void I2C0_IRQHandler(void);
#endif /* SRC_I2C_H_ */
