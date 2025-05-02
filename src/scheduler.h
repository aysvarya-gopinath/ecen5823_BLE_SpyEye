/*
 * scheduler.h
 *
 *  Created on: Feb 4, 2025
 * This file contains the functions of the event scheduler
    Author:@Aysvarya Gopinath   Aysvarya.Gopinath@Colorado.edu
 */

#ifndef SRC_SCHEDULER_H_
#define SRC_SCHEDULER_H_
#include "stdint.h"
#include"sl_bt_api.h"
#define NO_EVENT        (0)// Value for no event

#define UF_LETIMER     (0x01) //3s underflow
#define IRQ_WAIT_OVER    (0x02) //non-blocking wait
#define I2C_COMPLETE    (0x04) //i2c transfer complete
#define PB0_press   (0x08)  // External push button 0 press
#define PB0_release (0x10)  // External push button 0 release
#define PB1_press   (0x20)  // External push button 1 press
#define PB1_release (0x40)  // External push button 1 release
#define EXT_INTERRUPT (0x80) //external gpio interrupt from ambinet light sensor

//state variables of the temperature state machine
typedef enum uint32_t {
  Idle,
  PowerMode,
  WaitMode,
  SensorInit,
  SensorRead,
  SensorOFF,
  ConversionMode,
  } State_t;


/****************************************************************************
 * Set Event scheduler for the timer
 *****************************************************************************/

//set an underflow event
void schedulerSetEvent_underflow(void);

//set an comp1 interrupt event
void schedulerSetEvent_WaitIrq(void);

//set an i2c transfer complete event
void schedulerSetEvent_i2cTransfer(void);

//set an external push button 0 event for press and release
void schedulerSetEvent_PB0_release();
void schedulerSetEvent_PB0_press();

//set an external push button 1 event for press and release
void schedulerSetEvent_PB1_press();
void schedulerSetEvent_PB1_release();

//set an external ambinet light sensor event
void schedulerSetEvent_Ext_interrupt();

//state machine for the ambient light sensor
void VEML6030_state_machine(sl_bt_msg_t *evt);

//get the events set by the handler
uint32_t getNextEvent(void);


#endif /* SRC_SCHEDULER_H_ */
