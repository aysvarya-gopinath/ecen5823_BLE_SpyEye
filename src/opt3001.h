/*
 * opt3001.h
 *
 *  Created on: Apr 18, 2025
 *      Author: aysva
 */

#ifndef SRC_OPT3001_H_
#define SRC_OPT3001_H_
#include "sl_i2cspm.h"
#include"gpio.h"
#include"em_i2c.h"
#include <stdint.h>
#include <math.h>

// I2C address of OPT3001
#define OPT3001_I2C_ADDR    0x44  // 7-bit address

// OPT3001 Registers
#define OPT3001_REG_RESULT    0x00  //  result register address which holds the light measurement
#define OPT3001_REG_CONFIG    0x01  // configuration register address

void opt3001_init(void);
void opt3001_read_data(void);
void opt3001_conversion(void);
void opt3001_I2C_IRQHandler(void);
void opt3001_write_register(uint8_t reg, uint16_t value);
#endif // OPT3001_H
