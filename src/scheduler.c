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
// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

uint32_t  myEvents=NO_EVENT,setEvent=NO_EVENT; //variables to set events
// health thermometer service UUID
   const uint8_t service_uuid[2] = {0x09, 0x18}; //in little endian
// temperature Measurement characteristic UUID
   const uint8_t characterictic_uuid[2] = {0x1c, 0x2a};

 /*Scheduler routine to set scheduler event based on the underflow interrupt
 * No return types and parameters
 */
void schedulerSetEvent_underflow() {
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();//enter critical section
  sl_bt_external_signal(UF_LETIMER);//signal the Bluetooth stack that an UF occurred
  CORE_EXIT_CRITICAL(); // exit critical section
}

/*Scheduler routine to set an scheduler event based on the COMP1 interrupt(non-blocking)
 * No return types and parameters
 */
void schedulerSetEvent_WaitIrq(){
  CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();//enter critical section
    sl_bt_external_signal(IRQ_WAIT_OVER);//signal the Bluetooth stack that COMP1 overflowed (non-blocking wait)
    CORE_EXIT_CRITICAL(); // exit critical section
}

/*Scheduler routine to set an scheduler event based on i2c transfer interrupt
 * No return types and parameters
 */
void schedulerSetEvent_i2cTransfer(){
  CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();//enter critical section
    sl_bt_external_signal(I2C_COMPLETE); //signal the Bluetooth stack that i2c transfer was complete
    CORE_EXIT_CRITICAL(); // exit critical section
}



/*State machine to read temperature from Si7021 sensor via I2C
 * An EVENT set by the interrupts is passed as a parameter
 * No return type
 */
#if DEVICE_IS_BLE_SERVER
void Si7021_state_machine(sl_bt_msg_t *evt)
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
                  gpioSi7021ON (); //power on the sensor
                  timerWaitUs_irq (80000); //power up sequence delay
                  nextState = I2Cwrite;
                  LOG_INFO("Sensor on_D\n\r");
                }
              break;
            case I2Cwrite:
              nextState = I2Cwrite;
              if (eventValue && IRQ_WAIT_OVER) //non-blocking irq wait generates a COMP1 interrupt when expired
                {
                  sl_power_manager_add_em_requirement (SL_POWER_MANAGER_EM1); //add power requirement for EM1
                  send_I2C_command (); //write i2c command
                  nextState = SensorWait;
                  LOG_INFO("I2C command sent_D\n\r");
                }
              break;
            case SensorWait:
              nextState = SensorWait;
              if (eventValue && I2C_COMPLETE) //i2c write command is completed
                {
                  sl_power_manager_remove_em_requirement (SL_POWER_MANAGER_EM1); //Remove Power Req of EM1
                  timerWaitUs_irq (10800); //conversion delay
                  nextState = I2Cread;
                }
              break;
            case I2Cread:
              nextState = I2Cread; //i2c reading temperature data
              if (eventValue && IRQ_WAIT_OVER) //non-blocking irq wait generates a COMP1 interrupt when expired
                {
                  sl_power_manager_add_em_requirement (SL_POWER_MANAGER_EM1); //add power requirement for EM1
                  read_temp_data (); //i2c read temperature
                  LOG_INFO("I2C data read_D\n\r");
                  nextState = SensorOFF;
                }
              break;
            case SensorOFF:
              nextState = SensorOFF;
              if (eventValue && I2C_COMPLETE) //i2c read command is completed
                {
                  sl_power_manager_remove_em_requirement (SL_POWER_MANAGER_EM1); //Remove Power Req of EM1
                  log_temperature (); //log the temperature
                  nextState = Idle;
                }
              break;
            default:
              break;

            } // switch
        } //external_signal header

} //state machine
#else
/*The discovery state machine performs the services and characteristics discovery operations
 *
 */
void discovery_state_machine(sl_bt_msg_t *evt)
{
  sl_status_t sc;
  ble_data_struct_t *ble_data_ptr = get_ble_dataPtr(); //pointer to the ble_data structure
    static discoverState_t nextState = IDLE_STATE;
    discoverState_t current_state = nextState;
    switch (current_state)
    {
    case  IDLE_STATE://idle state
        nextState =  IDLE_STATE;
        if (SL_BT_MSG_ID(evt->header) == sl_bt_evt_connection_opened_id)
        {
            // discover primary services by UUID
            sc = sl_bt_gatt_discover_primary_services_by_uuid(ble_data_ptr->connectionHandle, sizeof(service_uuid), (uint8_t *)service_uuid);
            if (sc != SL_STATUS_OK)
              {
              LOG_ERROR("sl_bt_gatt_discover_primary_services_by_uuid() returned != 0 status=0x%04x", (unsigned int) sc);
              }
            LOG_INFO("services discovery by client\n\r");
            nextState = SERVICES_FOUND;
        }
        break;

    case SERVICES_FOUND://discovered services by uuid
     nextState = SERVICES_FOUND;
        if (SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
        {
            // discover  characteristics by specified UUID
       sc = sl_bt_gatt_discover_characteristics_by_uuid(ble_data_ptr->connectionHandle, ble_data_ptr->serviceHandle, sizeof(characterictic_uuid), (uint8_t *)characterictic_uuid);
            if (sc != SL_STATUS_OK)
                         {
                         LOG_ERROR("sl_bt_gatt_discover_characteristics_by_uuid() returned != 0 status=0x%04x", (unsigned int) sc);
                         }
            LOG_INFO("_characteristics discovery by client\n\r");
            nextState = CHARACTERITICS_FOUND;
        }
        break;

    case CHARACTERITICS_FOUND:// discovered characterictics by uuid
        nextState = CHARACTERITICS_FOUND;
        if (SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
        {
            // enable the indications sent from server
           sc = sl_bt_gatt_set_characteristic_notification(ble_data_ptr->connectionHandle, ble_data_ptr->characteristicsHandle, sl_bt_gatt_indication);
            if (sc != SL_STATUS_OK)
                                   {
                                   LOG_ERROR("sl_bt_gatt_set_characteristic_notification() returned != 0 status=0x%04x", (unsigned int) sc);
                                   }
            LOG_INFO("indications enabled by client\n\r");
            nextState = INDICATIONS_ENABLED;
        }
        break;

    case INDICATIONS_ENABLED: //gatt characteristics notifications enabled
        nextState = INDICATIONS_ENABLED;
        if (SL_BT_MSG_ID(evt->header) == sl_bt_evt_gatt_procedure_completed_id)
        {
            displayPrintf(DISPLAY_ROW_CONNECTION, "Handling Indications");//  client indicates
            nextState = WAIT_STATE;
            }
        else if (SL_BT_MSG_ID(evt->header) == sl_bt_evt_connection_closed_id)
                   {
                   nextState = INDICATIONS_ENABLED;
                   LOG_INFO("wait state client\n\r");
                   }
            break;

    case WAIT_STATE://waiting for external event
        nextState = WAIT_STATE;
        if (SL_BT_MSG_ID(evt->header) == sl_bt_evt_connection_closed_id)
        {
            nextState = INDICATIONS_ENABLED;
            LOG_INFO("closed client\n\r");
        }
        break;

    default:
      break;
    } //switch case
}//discovery state machine
#endif








