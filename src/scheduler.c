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
#include"sl_bt_api.h"
#include"ble.h"
#include"lcd.h"
#include "ble_device_type.h"
#include "veml6030.h"
// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 0
#include "src/log.h"

uint32_t myEvents = NO_EVENT, setEvent = NO_EVENT; //variables to set events


/*Scheduler routine to set scheduler event based on the underflow interrupt
 * No return types and parameters
 */
void schedulerSetEvent_underflow ()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL(); //enter critical section
  sl_bt_external_signal (UF_LETIMER); //signal the Bluetooth stack that an UF occurred
  CORE_EXIT_CRITICAL(); // exit critical section
}

/*Scheduler routine to set an scheduler event based on the COMP1 interrupt(non-blocking)
 * No return types and parameters
 */
void schedulerSetEvent_WaitIrq ()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL(); //enter critical section
  sl_bt_external_signal (IRQ_WAIT_OVER); //signal the Bluetooth stack that COMP1 overflowed (non-blocking wait)
  CORE_EXIT_CRITICAL(); // exit critical section
}

/*Scheduler routine to set an scheduler event based on i2c transfer interrupt
 * No return types and parameters
 */
void schedulerSetEvent_i2cTransfer ()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL(); //enter critical section
  sl_bt_external_signal (I2C_COMPLETE); //signal the Bluetooth stack that i2c transfer was complete
  CORE_EXIT_CRITICAL(); // exit critical section
}

/*Scheduler routine to set an scheduler event based on pushbutton 0 press interrupt
 * No return types and parameters
 */
void schedulerSetEvent_PB0_press()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL(); //enter critical section
  sl_bt_external_signal (PB0_press); //signal the Bluetooth stack that an (external interrupt)push button 0 pressed
  CORE_EXIT_CRITICAL(); // exit critical section
}


/*Scheduler routine to set an scheduler event based on pushbutton 1 press interrupt
 * No return types and parameters
 */
void schedulerSetEvent_PB1_press()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL(); //enter critical section
  sl_bt_external_signal (PB1_press); //signal the Bluetooth stack that an (external interrupt)push button 1 pressed
  CORE_EXIT_CRITICAL(); // exit critical section
}

/*Scheduler routine to set an scheduler event based on pushbutton 0 release interrupt
 * No return types and parameters
 */
void schedulerSetEvent_PB0_release()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL(); //enter critical section
  sl_bt_external_signal (PB0_release); //signal the Bluetooth stack that an (external interrupt)push button 0 release
  CORE_EXIT_CRITICAL(); // exit critical section
}


/*Scheduler routine to set an scheduler event based on pushbutton 1 release interrupt
 * No return types and parameters
 */
void schedulerSetEvent_PB1_release()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL(); //enter critical section
  sl_bt_external_signal (PB1_release); //signal the Bluetooth stack that an (external interrupt)push button 1 release
  CORE_EXIT_CRITICAL(); // exit critical section
}

/*Scheduler routine to set an scheduler event based on external ambient light sensor interrupt
 * No return types and parameters
 */
void schedulerSetEvent_Ext_interrupt()
{
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL(); //enter critical section
  sl_bt_external_signal (EXT_INTERRUPT); //signal the Bluetooth stack that an (external interrupt)from ambient light sensor
  CORE_EXIT_CRITICAL(); // exit critical section
}

/*State machine to read ambient light from the external VEML6030 sensor via I2C
 * An EVENT set by the interrupts is passed as a parameter
 * No return type
 */

                   /*****************************SERVER******************************/
void VEML6030_state_machine(sl_bt_msg_t *evt)
{
      if (SL_BT_MSG_ID(evt->header) == sl_bt_evt_system_external_signal_id) //external signals event is set
        {
          uint32_t eventValue = evt->data.evt_system_external_signal.extsignals; //holds all the enabled signals
          State_t currentState;
          static State_t nextState = Idle; //initial idle state
          currentState = nextState;
          switch (currentState)
            {
            case Idle:
              nextState = Idle; // initial idle state
              if (eventValue && UF_LETIMER) //if 3s UF interrupt
                {
                  sl_power_manager_add_em_requirement (SL_POWER_MANAGER_EM1);
                  veml6030_powerON(); //power on the sensor
                  nextState = PowerMode;
                }
              break;
            case PowerMode:
                         nextState = PowerMode;
                         if (eventValue && I2C_COMPLETE) //non-blocking irq wait generates a COMP1 interrupt when expired
                           {
                             veml6030_powerMode(); //set the low power mode
                             nextState = WaitMode;
                           }
                         break;
            case WaitMode:
                                   nextState = WaitMode;
                                   if (eventValue && I2C_COMPLETE) //non-blocking irq wait generates a COMP1 interrupt when expired
                                     {
                                       sl_power_manager_remove_em_requirement (SL_POWER_MANAGER_EM1);
                                       timerWaitUs_irq(4000); //4ms delay before read
                                       nextState = SensorInit;
                                     }
                                   break;

            case SensorInit:
              nextState =SensorInit;
              if (eventValue && IRQ_WAIT_OVER) //non-blocking irq wait generates a COMP1 interrupt when expired
                {
                  sl_power_manager_add_em_requirement (SL_POWER_MANAGER_EM1);
                  veml6030_init(); //initilaise the i2c for light sensor
                  nextState = SensorRead;
                }
              break;
/************ ATTEMPT TO CONFIGURE THE INTERRUPT FEATURE  ********************************/
 /*           case ReadWait:
                        nextState = ReadWait;
                        if (eventValue && I2C_COMPLETE) //non-blocking irq wait generates a COMP1 interrupt when expired
                          {
                            veml6030_high_threshold(); //set high threshold
                            nextState = ReadWaiting;
                          }
                        break;

            case ReadWaiting:
                                 nextState = ReadWaiting;
                                 if (eventValue && I2C_COMPLETE) //non-blocking irq wait generates a COMP1 interrupt when expired
                                   {
                                     veml6030_low_threshold();//set low threshold
                                     nextState = SensorWait;
                                   }
                                 break;


            case SensorWait:
              nextState = SensorWait;
              if (eventValue && I2C_COMPLETE)
                {
                  if  (eventValue && EXT_INTERRUPT) //only if the interrupt is set
                      config_read();          //read the config value
                  nextState = I2Cread;
                }
              break;

*/
            case SensorRead:
              nextState = SensorRead; //i2c reading temperature data
              if (eventValue && I2C_COMPLETE) //non-blocking irq wait generates a COMP1 interrupt when expired
                {
                  veml6030_read_data(); //read the sensor data
                  nextState =SensorOFF;
                }
              break;


            case SensorOFF:
              nextState = SensorOFF;
              if (eventValue && I2C_COMPLETE)
                {
                  veml6030_powerOFF();//turn off
                  nextState = SensorOFF;
                }
              break;

            case ConversionMode:
              nextState = ConversionMode;
              if (eventValue && I2C_COMPLETE) //i2c read command is completed
                {
                  sl_power_manager_remove_em_requirement (SL_POWER_MANAGER_EM1); //Remove Power Req of EM1
                  veml6030_conversion(); //convert raw data to lux
                  nextState = Idle;
                }
              break;
            default:
              break;

            } // switch
        } //external_signal header

} //state machine


