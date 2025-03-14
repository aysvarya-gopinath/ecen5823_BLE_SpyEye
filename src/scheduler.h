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
#define PUSH_BUTTON_PRESS  (0x08) //external push button 0

//state variables of the temperature state machine
typedef enum uint32_t {
  Idle,     //idle state
  I2Cwrite,   //i2c write occurs
  SensorWait,  // sensor waits
  I2Cread,   //i2c read has occurs
 SensorOFF,  //sensor is turned off
  } State_t;

  //state variables of the discovery state machine
  typedef enum uint16_t{
    IDLE_STATE,     //idle state
    SERVICES_FOUND,   //discovered services by uuid
    CHARACTERITICS_FOUND,  // discovered characterictics by uuid
    INDICATIONS_ENABLED,   //gatt characteristics notifications enabled
    WAIT_STATE,  //waiting for external event
    } discoverState_t;



/****************************************************************************
 * Set Event scheduler for the timer
 *****************************************************************************/

//set an underflow event
void schedulerSetEvent_underflow(void);

//set an comp1 interrupt event
void schedulerSetEvent_WaitIrq(void);

//set an i2c transfer complete event
void schedulerSetEvent_i2cTransfer(void);

//set an external push button 0 event
void schedulerSetEvent_pushbutton0();

//state machine for the temperature read
void Si7021_state_machine(sl_bt_msg_t *evt);

//get the events set by the handler
uint32_t getNextEvent(void);

//discovery state machine
void discovery_state_machine(sl_bt_msg_t *evt);

#endif /* SRC_SCHEDULER_H_ */
