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
#define VEML6030_REG_CONFIG    (0x00)  // command code configuration register address
#define VEML6030_REG_RESULT  (0x04)   //command code for data output register
#define VEML6030_REG_POWER    (0x03)  // command code for power saving mode register
#define VEML6030_REG_HIGH    (0x01)  // command code for high threshold register
#define VEML6030_REG_LOW    (0x02)  // command code for low threshold register
#define VEML6030_REG_INTERRUPT   (0x06)  // command code for interrupt read register


#define CONFIG_CODE (0x18C0)//  // gain is 1x; 100ms integration time
#define POWER_UP (0x00) //power on
#define POWER_MODE (0x07) // power mode 4 and power save enabled
#define LOW_THRESHOLD (0x0A2C)//1010001011000 for 300lux
#define HIGH_THRESHOLD (0x54C5)//for lux above 2500lux

void veml6030_init(void);
void veml6030_read_data(void);
void veml6030_conversion(void);
void veml6030_I2C_IRQHandler(void);
void veml6030_write_register(uint8_t reg, uint16_t value);
void i2c_scan_bus(void);
void config_read(void);
void veml6030_powerON(void);
void veml6030_powerMode(void);
void veml6030_low_threshold(void);
void veml6030_high_threshold(void);
#endif // VEML6030_H
