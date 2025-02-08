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

#define NO_EVENT           (0)// Value for no event

#define READ_TEMPERATURE  (0x02) //measure temperature set

// Used for signaling events
//static volatile uint32_t schedule_event;

/****************************************************************************
 * Set Event scheduler the timer
 *****************************************************************************/
void schedulerSetEvent_ReadTemp(void);

uint32_t getNextEvent(void);

#endif /* SRC_SCHEDULER_H_ */
