/*
  scheduler.c
  This file contains the functions of the event scheduler
  Created on: Feb 3, 2025
   Author:@Aysvarya Gopinath   Aysvarya.Gopinath@Colorado.edu
 *
 */
#include "scheduler.h"
#include "gpio.h"
#include"em_core_generic.h"
#include"em_core.h"
#include"timers.h"
#include"i2c.h"
#include"sl_power_manager.h"
// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

uint32_t  myEvents=NO_EVENT,setEvent=NO_EVENT; //variables to set events

//state variables of the state machine
typedef enum uint32_t {
  Idle,     //idle state
  I2Cwrite,   //i2c write occurs
  SensorWait,  // sensor waits
  I2Cread,   //i2c read has occurs
 SensorOFF,  //sensor is turned off
  } State_t;


/*Scheduler routine to set scheduler event based on the underflow interrupt
 * No return types and parameters
 */
void schedulerSetEvent_underflow() {
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();//enter critical section
  myEvents |= UF_LETIMER;//take measurement
  CORE_EXIT_CRITICAL(); // exit critical section
}

/*Scheduler routine to set an scheduler event based on the COMP1 interrupt(non-blocking)
 * No return types and parameters
 */
void schedulerSetEvent_WaitIrq(){
  CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();//enter critical section
    myEvents |=IRQ_WAIT_OVER;//non-blocking wait
    CORE_EXIT_CRITICAL(); // exit critical section
}

/*Scheduler routine to set an scheduler event based on i2c transfer interrupt
 * No return types and parameters
 */
void schedulerSetEvent_i2cTransfer(){
  CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();//enter critical section
    myEvents |=I2C_COMPLETE;//non-blocking wait
    CORE_EXIT_CRITICAL(); // exit critical section
}


/*Scheduler routine to return the event set by interrupts to process
 * No return types and parameters
 */
uint32_t getNextEvent() {
  CORE_DECLARE_IRQ_STATE;
  //take measurement every 3seconds when underflow occurs
  if (myEvents & UF_LETIMER)
    {
      setEvent = UF_LETIMER;
      CORE_ENTER_CRITICAL(); //enter critical section
      myEvents &= ~UF_LETIMER; // clear the bit after processing
      CORE_EXIT_CRITICAL(); // exit critical section
    }
  //timer non-blocking wait
  else if (myEvents & IRQ_WAIT_OVER)
    {
      setEvent = IRQ_WAIT_OVER;
      CORE_ENTER_CRITICAL(); //enter critical section
      myEvents &= ~IRQ_WAIT_OVER; // clear the bit after processing
      CORE_EXIT_CRITICAL(); // exit critical section
    }
  //i2c transfer complete
  else if (myEvents & I2C_COMPLETE)
    {
      setEvent = I2C_COMPLETE;
      CORE_ENTER_CRITICAL(); //enter critical section
      myEvents &= ~I2C_COMPLETE; // clear the bit after processing
      CORE_EXIT_CRITICAL(); // exit critical section
    }
  return (setEvent);
}

/*State machine to read temperature from Si7021 sensor via I2C
 * An EVENT set by the interrupts is passed as a parameter
 * No return type
 */
void Si7021_state_machine(uint32_t event)
{
  State_t currentState;
  static State_t nextState = Idle; //initial idle state
  currentState = nextState;
  switch (currentState)
    {
    case Idle:
      nextState = Idle; // initial idle state
      if (event == UF_LETIMER) //if 3s UF interrupt
        {
          gpioSi7021ON (); //power on the sensor
          timerWaitUs_irq (80000); //power up sequence delay
          nextState = I2Cwrite;
        }
      break;
    case I2Cwrite:
      nextState = I2Cwrite;
      if (event == IRQ_WAIT_OVER) //non-blocking irq wait generates a COMP1 interrupt when expired
        {
          sl_power_manager_add_em_requirement (SL_POWER_MANAGER_EM1); //add power requirement for EM1
          send_I2C_command (); //write i2c command
          nextState = SensorWait;
        }
      break;
    case SensorWait:
      nextState = SensorWait;
      if (event == I2C_COMPLETE)//i2c write command is completed
        {
          sl_power_manager_remove_em_requirement (SL_POWER_MANAGER_EM1); //Remove Power Req of EM1
          timerWaitUs_irq (10800); //conversion delay
          nextState = I2Cread;
        }
      break;
    case I2Cread:
      nextState = I2Cread; //i2c reading temperature data
      if (event == IRQ_WAIT_OVER) //non-blocking irq wait generates a COMP1 interrupt when expired
        {
          sl_power_manager_add_em_requirement (SL_POWER_MANAGER_EM1); //add power requirement for EM1
          read_temp_data (); //i2c read temperature
          nextState = SensorOFF;
        }
      break;
    case SensorOFF:
      nextState = SensorOFF;
      if (event == I2C_COMPLETE) //i2c read command is completed
        {
          gpioSi7021OFF (); //power off the sensor
          sl_power_manager_remove_em_requirement (SL_POWER_MANAGER_EM1); //Remove Power Req of EM1
          log_temperature (); //log the temperature
          nextState = Idle;
        }
      break;
    default:

      break;

    } // switch
} //state machine







