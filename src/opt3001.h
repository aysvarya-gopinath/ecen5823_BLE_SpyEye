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

#define SCL_pin (10) //pc11
#define SDA_pin (11 ) //pc10
// I2C address of OPT3001
#define OPT3001_I2C_ADDR    0x44  // 7-bit address

// OPT3001 Registers
#define OPT3001_REG_RESULT    0x00  // Light measurement result
#define OPT3001_REG_CONFIG    0x01  // Configuration register

// Configuration register values (for continuous mode)
#define OPT3001_CONFIG_DEFAULT 0x10C0  // Continuous conversion mode

extern uint8_t read_data[2];    // Buffer to hold sensor data
extern uint16_t light_data;     // Variable to store light level (lux)

void opt3001_init(void);
void opt3001_read_data(void);
void opt3001_send_command(uint8_t command);
void opt3001_I2C_IRQHandler(void);

#endif // OPT3001_H


#endif /* SRC_OPT3001_H_ */
