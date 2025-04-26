/*
 * VEML6030.h
 *
 *  Created on: Apr 18, 2025
 *      Author: aysva
 */

#ifndef SRC_VEML6030_H_
#define SRC_VEML6030_H_
#include "sl_i2cspm.h"
#include"gpio.h"
#include"em_i2c.h"
#include <stdint.h>
#include <math.h>

// I2C address of VEML6030
#define VEML6030_I2C_ADDR    0x48  // 7-bit address

// VEML6030 Registers
#define VEML6030_REG_CONFIG    0x00  // command code configuration register address
#define VEML6030_REG_RESULT   0x04   //command code for data output register

void veml6030_init(void);
void veml6030_read_data(void);
void veml6030_conversion(void);
void veml6030_I2C_IRQHandler(void);
void veml6030_write_register(uint8_t reg, uint16_t value);
void i2c_scan_bus(void);
#endif // VEML6030_H
