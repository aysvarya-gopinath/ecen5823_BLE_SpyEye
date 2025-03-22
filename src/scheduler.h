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
#define PUSH_BUTTON0  (0x08) //external push button 0
#define PUSH_BUTTON1  (0x10) //external push button 1

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
    THERMO_SERVICES_DISCOVER,   //discovered services by uuid for temperature
    THERMO_CHARACTERITICS_DISCOVER,  // discovered characterictics by uuid
    THERMO_INDICATIONS_ENABLE,   //gatt characteristics notifications enabled
    PB_SERVICES_DISCOVER,   //discovered services by uuid for push button
    PB_CHARACTERITICS_DISCOVER,  // discovered characterictics by uuid
    PB_INDICATIONS_ENABLE,   //gatt characteristics notifications enabled
    WAIT_STATE,  //waiting for external event
    CLOSED_STATE, //connection closed
    } discoverState_t;

#if !DEVICE_IS_BLE_SERVER
// health thermometer service UUID
    static const uint8_t thermo_service[2] ={ 0x09, 0x18 }; //in little endian
// temperature Measurement characteristic UUID
    static const uint8_t thermo_char[2] ={ 0x1c, 0x2a };

    static const uint8_t pb_service[16] ={0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e,
                                  0x43, 0xc8, 0x38,0x01, 0x00, 0x00, 0x00};
    static const uint8_t pb_char[16] ={0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38,
     0x02, 0x00, 0x00, 0x00};

#endif


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

//set an external push button 1 event
void schedulerSetEvent_pushbutton1();

//state machine for the temperature read
void Si7021_state_machine(sl_bt_msg_t *evt);

//get the events set by the handler
uint32_t getNextEvent(void);

//discovery state machine
void discovery_state_machine(sl_bt_msg_t *evt);

#endif /* SRC_SCHEDULER_H_ */
