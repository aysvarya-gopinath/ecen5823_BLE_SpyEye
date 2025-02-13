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

#define NO_EVENT        (0)// Value for no event

#define UF_LETIMER     (0x01) //3s underflow
#define IRQ_WAIT_OVER    (0x02) //non-blocking wait
#define I2C_COMPLETE    (0x04) //i2c transfer complete

/****************************************************************************
 * Set Event scheduler for the timer
 *****************************************************************************/

//set an underflow event
void schedulerSetEvent_underflow(void);

//set an comp1 interrupt event
void schedulerSetEvent_WaitIrq(void);

//set an i2c transfer complete event
void schedulerSetEvent_i2cTransfer(void);

//state machine for the temperature read
void Si7021_state_machine(uint32_t event);

//get the events set by the handler
uint32_t getNextEvent(void);

#endif /* SRC_SCHEDULER_H_ */
