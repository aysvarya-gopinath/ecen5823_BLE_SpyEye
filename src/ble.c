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
#include"math.h"
#include "lcd.h"
#include"ble.h"
#include"ble_device_type.h"
ble_data_struct_t ble_data;// BLE private data


/* This function returns a pointer to the ble_data structure
 * No parameters
 * Returns a pointer to the ble data structure
 */
ble_data_struct_t* get_ble_dataPtr(void) {
    return &ble_data;
}
bd_addr bt_address = SERVER_BT_ADDRESS;

/*This function sends temperature data over BLE
 * No return types and parameters
 */
#if DEVICE_IS_BLE_SERVER
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

  LOG_INFO("waiting for htm indications to send temperature to client");
  if (ble_data.connect_open == true && ble_data.htm_indications == true)
    { //if connections are open and htm indications are enabled
      if (!ble_data.inflight_indication) //not in transit
        {
          LOG_INFO("sending temperature to client");
          displayPrintf(DISPLAY_ROW_TEMPVALUE,"Temperature=%u",temp_deg); //disaply the temperature on screen
          //send indication with temperature
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
              ble_data.inflight_indication = INFLIGHT; //mark as in transit
            }
        }
    }
}

#else
static int32_t FLOAT_TO_INT32(const uint8_t *buffer_ptr)
{
  uint8_t     signByte = 0;
  int32_t     mantissa;
    // input data format is:
    // [0]       = flags byte, bit[0] = 0 -> Celsius; =1 -> Fahrenheit
    // [3][2][1] = mantissa (2's complement)
    // [4]       = exponent (2's complement)
  // BT buffer_ptr[0] has the flags byte
  int8_t exponent = (int8_t)buffer_ptr[4];
  // sign extend the mantissa value if the mantissa is negative
    if (buffer_ptr[3] & 0x80) { // msb of [3] is the sign of the mantissa
      signByte = 0xFF;
    }
  mantissa = (int32_t) (buffer_ptr[1]  << 0)  |
                       (buffer_ptr[2]  << 8)  |
                       (buffer_ptr[3]  << 16) |
                       (signByte       << 24) ;
  // value = 10^exponent * mantissa, pow() returns a double type
  return (int32_t) (pow(10, exponent) * mantissa);
} // FLOAT_TO_INT32

#endif


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
/////////////////////////////////////RADIO HAS STARTED//////////////////////////////////////////////////////////////////
    case sl_bt_evt_system_boot_id:
      {
        displayInit ();  //initialise the lcd display
        // extract BT device address
              sc = sl_bt_system_get_identity_address (&ble_data.myAddress,
                                                      &ble_data.myAddressType);
              if (sc != SL_STATUS_OK)
                {
                  LOG_ERROR(
                      "\n\r sl_bt_system_get_identity_address() returned != 0 status=0x%04x\n",
                      (unsigned int) sc);
                }
              else{
                  //displays the BT stack address on the lcd in little endian
                 displayPrintf (DISPLAY_ROW_BTADDR, "%02X:%02X:%02X:%02X:%02X:%02X",
                                 ble_data.myAddress.addr[0], ble_data.myAddress.addr[1],
                                 ble_data.myAddress.addr[2], ble_data.myAddress.addr[3],
                                 ble_data.myAddress.addr[4], ble_data.myAddress.addr[5]);
                  displayPrintf (DISPLAY_ROW_ASSIGNMENT, ASSIGNMENT); //displays assignment number
              }
         /*********************************SERVER**************************************************/
#if DEVICE_IS_BLE_SERVER

        //create advertising set for the slave device to advertise
        sc = sl_bt_advertiser_create_set (&ble_data.advertisingSetHandle);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("\n\r sl_bt_advertiser_create_set() returned != 0 status=0x%04x\n",(unsigned int) sc);
          }
        //set the timing parameters for advertising
        sc = sl_bt_advertiser_set_timing (ble_data.advertisingSetHandle,
                                          ADVERTISING_MIN,
                                          ADVERTISING_MAX,
                                          DURATION,
                                          MAX_EVENTS); //default durations
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
            LOG_ERROR("\n\r sl_bt_legacy_advertiser_generate_data() returned != 0 status=0x%04x\n",(unsigned int) sc);
          }
        //start sending advertising packets
        sc = sl_bt_legacy_advertiser_start (
            ble_data.advertisingSetHandle, sl_bt_legacy_advertiser_connectable);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("\n\r sl_bt_legacy_advertiser_start() returned != 0 status=0x%04x\n",(unsigned int) sc);
          }

        displayPrintf (DISPLAY_ROW_NAME, BLE_DEVICE_TYPE_STRING); // prints Server on display
        LOG_INFO("server advertising\n\r");
        displayPrintf (DISPLAY_ROW_CONNECTION, "Advertising");

     /***************************************CLIENT************************************/
#else
      //set scanner parameters for the subsequent operations
      sc= sl_bt_scanner_set_parameters(sl_bt_scanner_scan_mode_passive,SCAN_INTERVAL,SCAN_WINDOW);//passive scanning
      if (sc != SL_STATUS_OK)
                {
                  LOG_ERROR(
                      "\n\r sl_bt_scanner_set_parameters() returned != 0 status=0x%04x\n",
                      (unsigned int) sc);
                }
      //set the default bluetooth connection parameters
       sc= sl_bt_connection_set_default_parameters(MIN_INTERVAL,MAX_INTERVAL,LATENCY,TIMEOUT,MIN_CE_LEN,MAX_CE_LEN);
       if (sc != SL_STATUS_OK)
                 {
                   LOG_ERROR(
                       "\n\r sl_bt_connection_set_default_parameters() returned != 0 status=0x%04x\n",
                       (unsigned int) sc);
                 }
       //start scanning for devices
        sc=sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m,sl_bt_scanner_discover_generic);
        if (sc != SL_STATUS_OK)
                  {
                    LOG_ERROR(
                        "\n\r sl_bt_scanner_start() returned != 0 status=0x%04x\n",
                        (unsigned int) sc);
                  }
        else{
            displayPrintf (DISPLAY_ROW_NAME, BLE_DEVICE_TYPE_STRING); // prints Server on display
            LOG_INFO("client scanning\n\r");
            displayPrintf (DISPLAY_ROW_CONNECTION, "Discovering");
        }

#endif
        break;
      }

/////////////////////////////////////CONNECTION OPENED/////////////////////////////////////////////////////////
    case sl_bt_evt_connection_opened_id:  //new connection has opened
      { LOG_INFO("connection opened\n\r");
        displayPrintf (DISPLAY_ROW_CONNECTION, "Connected");
        ble_data.connectionHandle = evt->data.evt_connection_opened.connection; //connection handle

        /**********************************SERVER*********************************************/
#if DEVICE_IS_BLE_SERVER
        ble_data.connect_open = true; //connection is opened
        sc = sl_bt_advertiser_stop (ble_data.advertisingSetHandle); //stop advertising
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR(
                "\n\r sl_bt_advertiser_stop() returned != 0 status=0x%04x\n",
                (unsigned int) sc);
          }
        //request parameters
        sc = sl_bt_connection_set_parameters (ble_data.connectionHandle,
                                              MIN_INTERVAL,
                                              MAX_INTERVAL,
                                              LATENCY,
                                              TIMEOUT,
                                              0,
                                              4); //default max and min connection event length
        if (sc != SL_STATUS_OK)
          LOG_ERROR(
              "\n\r sl_bt_connection_set_parameters != 0 status=0x%04x \n",
              (unsigned int) sc);
        LOG_INFO("server set connection paarmeters and stop advertising\n\r");

        /**********************************CLIENT***********************************/
#else

        //displays the BT stack address of the server
        displayPrintf (DISPLAY_ROW_BTADDR2, "%02X:%02X:%02X:%02X:%02X:%02X",
                                             bt_address.addr[0], bt_address.addr[1],
                                             bt_address.addr[2], bt_address.addr[3],
                                             bt_address.addr[4], bt_address.addr[5]);
#endif
        break;
      }

///////////////////////////////////////LAZY SOFT TIMER EXPIRY////////////////////////////////////////////////////////
    case sl_bt_evt_system_soft_timer_id : //soft timer expired
      {
        displayUpdate();  //refresh the lcd to prevent charge buildup
        break;
      }

////////////////////////////////////////CONNECTION CLOSED//////////////////////////////////////////////////////////
    case sl_bt_evt_connection_closed_id: //connection has closed
      {
        /*****************************SERVER******************************/
#if DEVICE_IS_BLE_SERVER
        ble_data.connect_open = false; //connection is closed
        ble_data.inflight_indication = false; //not inflight
        ble_data.htm_indications = false; //
        LOG_INFO("connection closed\n\r");
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
        //start advertising again
        sc = sl_bt_legacy_advertiser_start (
            ble_data.advertisingSetHandle, sl_bt_legacy_advertiser_connectable);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR(
                "\n\r sl_bt_legacy_advertiser_start() returned != 0 status=0x%04x\n",
                (unsigned int) sc);
          }
        else{
        displayPrintf (DISPLAY_ROW_CONNECTION, "Advertising");
        displayPrintf (DISPLAY_ROW_TEMPVALUE, ""); //clear the temperature value
        }

    /*****************************CLIENT******************************/
#else
             //start scanning for devices
                sc=sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m,sl_bt_scanner_discover_generic);
                if (sc != SL_STATUS_OK)
                          {
                            LOG_ERROR(
                                "\n\r sl_bt_scanner_start() returned != 0 status=0x%04x\n",
                                (unsigned int) sc);
                          }
                else
                  {         LOG_INFO("client scanning\n\r");
                             displayPrintf( DISPLAY_ROW_CONNECTION, "Discovering" );
                             displayPrintf(DISPLAY_ROW_BTADDR2, "");
                             displayPrintf( DISPLAY_ROW_TEMPVALUE, "" );
                  }
#endif
        break;
      }

      /*****************************SERVER******************************/
#if DEVICE_IS_BLE_SERVER
/////////////////// /////////////////////CONNECTION PARAMETERS////////////////////////////////////////////
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
/////////////////////////////////////////GATT SERVER CHARACTERSITICS/////////////////////////////////////
    case sl_bt_evt_gatt_server_characteristic_status_id:
      {
        LOG_INFO("gatt server characteristics by server\n\r");
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
                  {
                    ble_data.htm_indications = true;  //enabled htm indications
                  } // sl_bt_gatt_server_indication
                else
                  {
                    ble_data.htm_indications = false;  //disabled htm indication
                    displayPrintf (DISPLAY_ROW_TEMPVALUE, "");
                  }
              }  //sl_bt_gatt_server_client_config
            if (evt->data.evt_gatt_server_characteristic_status.status_flags
                == sl_bt_gatt_server_confirmation) //client acknowledged indication
              {
                ble_data.inflight_indication = NOT_INFLIGHT; //clear inflight flag

              } //sl_bt_gatt_server_confirmation

          } //gattdb_temperature_measurement
        break;
      }
////////////////////////////////////////////SERVER TIMEOUT///////////////////////////////////////////////////////////
    case sl_bt_evt_gatt_server_indication_timeout_id:
      {
        LOG_INFO("timeout\n\r");
        ble_data.inflight_indication = NOT_INFLIGHT; // indication in flight
        break;
      }

      /*****************************CLIENT********************************/
#else
//////////////////////////////////////////CLIENT SCANNED DEVICES///////////////////////////////////////////////////
    case  sl_bt_evt_scanner_legacy_advertisement_report_id:
      {
                    //checking if its the matching predefined address
                 if ((evt->data.evt_scanner_legacy_advertisement_report.address.addr[0] == bt_address.addr[0]) &&
                     (evt->data.evt_scanner_legacy_advertisement_report.address.addr[1] == bt_address.addr[1]) &&
                     (evt->data.evt_scanner_legacy_advertisement_report.address.addr[2] == bt_address.addr[2]) &&
                     (evt->data.evt_scanner_legacy_advertisement_report.address.addr[3] == bt_address.addr[3]) &&
                     (evt->data.evt_scanner_legacy_advertisement_report.address.addr[4] == bt_address.addr[4]) &&
                     (evt->data.evt_scanner_legacy_advertisement_report.address.addr[5] == bt_address.addr[5]))//&&
                     //(evt->data.evt_scanner_legacy_advertisement_report.event_flags == SL_BT_SCANNER_EVENT_FLAG_SCANNABLE) && //checking if connectable
                   //(evt->data.evt_scanner_legacy_advertisement_report.address_type == sl_bt_gap_public_address))
                    {
                     printf("Received Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                                evt->data.evt_scanner_legacy_advertisement_report.address.addr[0],
                                evt->data.evt_scanner_legacy_advertisement_report.address.addr[1],
                                evt->data.evt_scanner_legacy_advertisement_report.address.addr[2],
                                evt->data.evt_scanner_legacy_advertisement_report.address.addr[3],
                                evt->data.evt_scanner_legacy_advertisement_report.address.addr[4],
                                evt->data.evt_scanner_legacy_advertisement_report.address.addr[5]);
     sc=sl_bt_scanner_stop();
     LOG_INFO("device matched client not scanning\n\r");
     if (sc != SL_STATUS_OK) {
         LOG_ERROR( "\n\r sl_bt_scanner_stop() returned != 0 status=0x%04x\n",
                                     (unsigned int) sc);
                               }

      sc=sl_bt_connection_open(bt_address,
                               sl_bt_gap_public_address,sl_bt_gap_phy_1m,
                               &ble_data.connectionHandle);  //wat connection handle??????????????????????????????????????????
      if (sc != SL_STATUS_OK) {
          LOG_ERROR(  "\n\r sl_bt_connection_open() returned != 0 status=0x%04x\n",
           (unsigned int) sc);
               }
      else
        {
          LOG_INFO("client connects\n\r");
        }
                    }
       break;
      }
///////////////////////////////////////////////////SERVICE DISCOVERED//////////////////////////////////////////////
    case sl_bt_evt_gatt_service_id:
          {
              ble_data.serviceHandle = evt->data.evt_gatt_service.service;//save the service handle
              LOG_INFO("service discovered \n\r");
          }

            break;
///////////////////////////////////////////////////CHARACTERICTICS DISCOVERED////////////////////////////////////////
        case sl_bt_evt_gatt_characteristic_id:
          {
              ble_data.characteristicsHandle = evt->data.evt_gatt_characteristic.characteristic; //save the characteristics handle
              LOG_INFO("characteristics discovered \n\r");
         }

            break;
///////////////////////////////////////////////CHARACTERICTIC VALUE RECEIVED//////////////////////////////////////////
        case sl_bt_evt_gatt_characteristic_value_id:
          {
            LOG_INFO("characteristic value discovered \n\r");
        //    if(evt->data.evt_gatt_characteristic_value.characteristic==ble_data.characteristicsHandle) //desired characteristics handle
            //  { //sending confirmation for the received indication from server
                sc = sl_bt_gatt_send_characteristic_confirmation(ble_data.connectionHandle);
                if (sc != SL_STATUS_OK) {
                                      LOG_ERROR("sl_bt_gatt_send_characteristic_confirmation() returned != 0 status=0x%04x", (unsigned int) sc);
                                  }
             // }
                int32_t temp_degree = FLOAT_TO_INT32(evt->data.evt_gatt_characteristic_value.value.data);
                LOG_INFO("Temp = %d\r", temp_degree);
                displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp = %d", temp_degree);
          }
            break;
#endif
    } // end - switch
}

