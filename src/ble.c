/*
 * ble.c
 *This file contains ble functions for the communication
 *  Created on: Feb 17, 2025
 *    Author:@Aysvarya Gopinath   Aysvarya.Gopinath@Colorado.edu
 */

#include "ble.h"
#include "sl_bt_api.h"
#include "gatt_db.h"
// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1 //
#include "src/log.h"

ble_data_struct_t ble_data;// BLE private data


/* This function returns a pointer to the ble_data structure
 * No parameters
 * Returns a pointer to the ble data structure
 */
ble_data_struct_t* get_ble_dataPtr(void) {
    return &ble_data;
}

/*Event responder to handle the bluetooth stack activity events
 *The external event is passed as a parameter
 *No returns types
 */
void handle_ble_event(sl_bt_msg_t *evt) {
  sl_status_t sc; // status code
  switch (SL_BT_MSG_ID(evt->header))
    {
  /******************************************************/
// Events common to both Servers and Clients
// ****************************************************** /
// --------------------------------------------------------
// This event indicates the device has started and the radio is ready.
// Do not call any stack API commands before receiving this boot event!
// Including starting BT stack soft timers!
// --------------------------------------------------------
   /////////////////RADIO HAS STARTED//////////////////////////////
    case sl_bt_evt_system_boot_id:
      {
        // extract BT device address
        sc = sl_bt_system_get_identity_address (&ble_data.myAddress,
                                                &ble_data.myAddressType);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR(
                "\n\r sl_bt_system_get_identity_address() returned != 0 status=0x%04x\n",
                (unsigned int) sc);
          }
        //create advertising set for the slave device to advertise
        sc = sl_bt_advertiser_create_set (&ble_data.advertisingSetHandle);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR(
                "\n\r sl_bt_advertiser_create_set() returned != 0 status=0x%04x\n",
                (unsigned int) sc);
          }
        //set the timing parameters for advertising
        sc = sl_bt_advertiser_set_timing (ble_data.advertisingSetHandle,
                                          ADVERTISING_MIN, ADVERTISING_MAX,
                                          DURATION, MAX_EVENTS); //default durations
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR(
                "\n\r sl_bt_advertiser_set_timing() returned != 0 status=0x%04x\n",
                (unsigned int) sc);
          }
        //generate advertising data
        sc = sl_bt_legacy_advertiser_generate_data (
            ble_data.advertisingSetHandle,
            sl_bt_advertiser_general_discoverable);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR(
                "\n\r sl_bt_legacy_advertiser_generate_data() returned != 0 status=0x%04x\n",
                (unsigned int) sc);
          }
        //start sending advertising packets
        sc = sl_bt_legacy_advertiser_start (
            ble_data.advertisingSetHandle, sl_bt_legacy_advertiser_connectable);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR(
                "\n\r sl_bt_legacy_advertiser_start() returned != 0 status=0x%04x\n",
                (unsigned int) sc);
          }
        break;
      }
    ///////////////////////////CONNECTION OPENED////////////////////////
    case sl_bt_evt_connection_opened_id:  //new connection has opened
      {
        ble_data.connect_open = true; //connection is opened
        ble_data.connectionHandle = evt->data.evt_connection_opened.connection; //connection handle
        sc = sl_bt_advertiser_stop (ble_data.advertisingSetHandle); //stop advertising
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR(
                "\n\r sl_bt_advertiser_stop() returned != 0 status=0x%04x\n",
                (unsigned int) sc);
          }
        //request parameters
        sc = sl_bt_connection_set_parameters (
            ble_data.connectionHandle,
            CONNECT_MIN_INTERVAL, CONNECT_MAX_INTERVAL, LATENCY, TIMEOUT, 0,
            0xffff); //default max and min connection event length
        if (sc != SL_STATUS_OK)
          LOG_ERROR(
              "\n\r sl_bt_connection_set_parameters != 0 status=0x%04x \n",
              (unsigned int) sc);
        break;
      }
      ///////////////////////////////CONNECTION CLOSED////////////////////////////////////
    case sl_bt_evt_connection_closed_id: //connection has closed
      {
        ble_data.connect_open = false; //connection is closed
        //generate advertising data
        sc = sl_bt_legacy_advertiser_generate_data (
            ble_data.advertisingSetHandle,
            sl_bt_advertiser_general_discoverable);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR(
                "\n\r sl_bt_legacy_advertiser_generate_data() returned != 0 status=0x%04x\n",
                (unsigned int) sc);
          }
        //start sending advertising packets
        sc = sl_bt_legacy_advertiser_start (
            ble_data.advertisingSetHandle, sl_bt_legacy_advertiser_connectable);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR(
                "\n\r sl_bt_legacy_advertiser_start() returned != 0 status=0x%04x\n",
                (unsigned int) sc);
          }
        break;
      }
      /////////////////////CONNECTION PARAMETERS////////////////////////////////////////////
    case sl_bt_evt_connection_parameters_id: //connection parameters set by master
      {
        LOG_INFO("\n\r MASTER SET CONNECTION PARAMETERS \n");
        LOG_INFO("\n\rConnection interval %d ms",
                 (int)((evt->data.evt_connection_parameters.interval)*1.25)); //connection interval
        LOG_INFO("\n\rSlave latency %u ms",
                 evt->data.evt_connection_parameters.latency); //peripheral latency
        LOG_INFO("\n\r Timeout %u ms",
                 (evt->data.evt_connection_parameters.timeout)*10); //supervision timeout
        break;
      }
      ////////////////GATT SERVER CHARACTERSITICS////////////////////////////
    case sl_bt_evt_gatt_server_characteristic_status_id:
      {
        //Track whether indications are enabled/disabled for each and every characteristics

        if (evt->data.evt_gatt_server_characteristic_status.characteristic
            == gattdb_temperature_measurement)
          {
            if (evt->data.evt_gatt_server_characteristic_status.status_flags
                == sl_bt_gatt_server_client_config)
              {
                if ((evt->data.evt_gatt_server_characteristic_status.client_config_flags
                    == sl_bt_gatt_server_indication)
                    || (evt->data.evt_gatt_server_characteristic_status.client_config_flags
                        == sl_bt_gatt_server_notification_and_indication))
                  ble_data.htm_indications = true;  //enabled htm indications
                else
                  ble_data.htm_indications = false;  //disabled htm indication
              }
            if (evt->data.evt_gatt_server_characteristic_status.status_flags
                == sl_bt_gatt_server_confirmation) //client acknowledged indication
              {
                ble_data.indication = NOT_INFLIGHT; //clear inflight flag
              }

          }
        break;
      }
      /////////////////SERVER TIMEOUT/////////////////////
    case sl_bt_evt_gatt_server_indication_timeout_id:
      {
        ble_data.indication = NOT_INFLIGHT; // indication in flight

        break;
      }
    } // end - switch
}

/*This function sends temperature data over BLE
 * No return types and parameters
 */
void send_temp_ble(int32_t temp_deg){
  sl_status_t sc;
  uint8_t htm_temperature_buffer[5]; //buffer to hold temperature
  uint8_t *p = htm_temperature_buffer;
  uint32_t htm_temperature_flt;
  uint8_t flags = 0x00;
  UINT8_TO_BITSTREAM(p, flags); // insert the flags byte
  htm_temperature_flt = INT32_TO_FLOAT(temp_deg*1000, -3);
  UINT32_TO_BITSTREAM(p, htm_temperature_flt); // insert the temperature measurement

//write temperature to gatt db
  sc = sl_bt_gatt_server_write_attribute_value (
  gattdb_temperature_measurement, // handle from gatt_db.h
      0,             // offset
      5,            // length
      htm_temperature_buffer      // in IEEE-11073 format
      );
  if (sc != SL_STATUS_OK)
    {
      LOG_ERROR(
          "\n\r sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x\n",
          (unsigned int) sc);
    }

  if (ble_data.connect_open == true && ble_data.htm_indications == true)
    { //if connections are open
      if (!ble_data.indication)//not in transit
        {   //send indication with temperature
          sc = sl_bt_gatt_server_send_indication (ble_data.connectionHandle,
          gattdb_temperature_measurement, // handle from gatt_db.h
                                                  5, htm_temperature_buffer // in IEEE-11073 format
                                                  );
          if (sc != SL_STATUS_OK)
            {
              LOG_ERROR(
                  "\n\r sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x\n",
                  (unsigned int) sc);
            }
          else
            {
              ble_data.indication = INFLIGHT; //mark as in transit
            }
        }
    }
}

