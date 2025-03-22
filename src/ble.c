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
#include "scheduler.h"
#include"gpio.h"

queue_struct_t   my_queue[QUEUE_DEPTH]; // the queue
uint32_t         wptr = 0;              // write pointer
uint32_t         rptr = 0;              // read pointer
uint32_t  buff_elements = 0;   //length of the enqueued buffer
bool full_flag=false;
bool empty_flag=false; // flags to indicate full and empty queue
bool isEmpty = false, isFull=false; //holds return types
ble_data_struct_t ble_data; // BLE private data
bd_addr bt_address = SERVER_BT_ADDRESS;

//    cbfifo variables
uint16_t     charHandle;
uint32_t     bufLength;
 uint8_t      buffer[5];


/* This function returns a pointer to the ble_data structure
 * No parameters
 * Returns a pointer to the ble data structure
 */
ble_data_struct_t* get_ble_dataPtr (void)
{
  return &ble_data;
}


/*Increments the read and write pointer to the next position
 * pointer is the parameter
 * No return types
 */
static uint32_t nextPtr(uint32_t ptr) {
  ptr++;
 if (ptr == QUEUE_DEPTH) // if the next pointer is equal to queue depth
 return 0; // wrap back to 0
 else
 return ptr; // advance
} // nextPtr()


// This function writes an entry to the queue if the the queue is not full.
// Input parameter "charHandle" should be written to queue_struct_t element "charHandle".
// Input parameter "bufLength" should be written to queue_struct_t element "bufLength"
// The bytes pointed at by input parameter "buffer" should be written to queue_struct_t element "buffer"
// Returns bool false if successful or true if writing to a full fifo.
// i.e. false means no error, true means an error occurred.
// ---------------------------------------------------------------------
bool write_queue (uint16_t charHandle, uint32_t bufLength, uint8_t *buffer) {

if (nextPtr(wptr) == rptr ) //then full
{
  full_flag=true;    // set the flag to indicate queue is full
  empty_flag=false;
 return true ;    // queue is full
  }
else              // queue not full
{
if(MIN_BUFFER_LENGTH<=bufLength&& bufLength<=MAX_BUFFER_LENGTH)//buflength in the range
{
my_queue[wptr].charHandle= charHandle; // write the data into the buffer
my_queue[wptr].bufLength=bufLength;
memcpy(my_queue[wptr].buffer, buffer, bufLength); // Copy the data to the buffer
buff_elements++; // increment when an element is written to the queue
wptr= nextPtr(wptr); //increment the write pointer
}
 full_flag = (nextPtr(wptr) == rptr);  //update the full flag status after writing
 empty_flag = false; //update the empty flag staus
return false; //queue is not full and write is successfull
}
}


// This function reads an entry from the queue, and returns values to the
// caller. The values from the queue entry are returned by writing
// the values to variables declared by the caller, where the caller is passing
// in pointers to charHandle, bufLength and buffer. The caller's code will look like this:
//
//   uint16_t     charHandle;
//   uint32_t     bufLength;
//   uint8_t      buffer[5];
//
//   status = read_queue (&charHandle, &bufLength, &buffer[0]);
// Returns bool false if successful or true if reading from an empty fifo.
// i.e. false means no error, true means an error occurred.
// ---------------------------------------------------------------------
bool read_queue (uint16_t *charHandle, uint32_t *bufLength, uint8_t *buffer) {
  if (wptr == rptr) //then empty
{
  empty_flag=true;    // set the flag to indicate queue is empty
     full_flag=false; // no operation
 return true ;    // queue is empty
  }
else            // queue not empty
{
     *charHandle = my_queue[rptr].charHandle; // read the data from the buffer
      *bufLength  = my_queue[rptr].bufLength;
      memcpy(buffer, my_queue[rptr].buffer, *bufLength); //block data transfer
        rptr = nextPtr(rptr); // increment the read pointer
  buff_elements--; // decrement when an element is read from the queue
 empty_flag = (wptr == rptr); //update the empty flag after reading
 full_flag = false;
  return false; // read successfully
}
}


// ---------------------------------------------------------------------
// Public function.
// Function that computes the number of written entries currently in the queue. If there
// are 3 entries in the queue, it should return 3. If the queue is empty it should
// return 0. If the queue is full it should return either QUEUE_DEPTH if
// USE_ALL_ENTRIES==1 otherwise returns QUEUE_DEPTH-1.
// ---------------------------------------------------------------------
uint32_t get_queue_depth() {
if (wptr==rptr) // if set, queue is empty
  {
return 0;
  }
  else if (nextPtr(wptr) == rptr) // if set , queue is full
  {
   return (QUEUE_DEPTH-1) ; // use all entries is 0
  }
  else if (wptr > rptr) { // queue not full, normal case
   return buff_elements;
  }
  else { // queue is wrapped around
    return (QUEUE_DEPTH - rptr + wptr);
  }

}



/*This function sends temperature data over BLE
 * No return types
 * raw temperature is the  parameters
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
  if (ble_data.htm_indications == true)//if the indications are enabled
    {
      if (ble_data.inflight_indication == false && (get_queue_depth () == 0)) //if not inflight and queue is empty
        {
          displayPrintf (DISPLAY_ROW_TEMPVALUE, "Temperature=%u", temp_deg); //display the temperature on screen
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
            { ble_data.inflight_indication = INFLIGHT; //mark as in transit
            LOG_INFO("sending data to the client for htm directly");
            }

        }
      else           //if queue is not empty and inflight
        {
          displayPrintf (DISPLAY_ROW_TEMPVALUE, "");  //clear temp
          LOG_INFO("writing data to the queue for htm");
          isEmpty =write_queue (gattdb_temperature_measurement, 5,
                       htm_temperature_buffer);
          if (isEmpty == true)
                     LOG_ERROR("circular buffer is full\n\r");


        }
}
}


/*This function sends push button data over BLE
 * No return types
 * push button state is the parameter
 */
void send_pushbutton_data(uint8_t pb_data)
{
  sl_status_t sc;
  sc = sl_bt_gatt_server_write_attribute_value (
      gattdb_button_state,
      0, //offset
      1, //length
      &pb_data);
  if (sc != SL_STATUS_OK)
    {
      LOG_ERROR(
          "\n\r sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x\n",
          (unsigned int) sc);
    }
  //check if the indications for the button press is enabled
  if (ble_data.button_state_indication == true)
    {
      if (ble_data.inflight_indication == false && (get_queue_depth () == 0)) //if queue is empty then send directly to client
        {
          sc = sl_bt_gatt_server_send_indication (ble_data.connectionHandle,
                                                  gattdb_button_state,
                                                  1,
                                                  &pb_data);
          if (sc != SL_STATUS_OK)
            {
              LOG_ERROR(
                  "\n\r sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x\n",
                  (unsigned int) sc);
            }
          else
            { ble_data.inflight_indication = true; // indications are inflight
            LOG_INFO("sending data to the client for button directly");
            }
        }
      else
        {
          isEmpty = write_queue (gattdb_button_state, sizeof(uint8_t),
                                 (uint8_t*) &pb_data);
          LOG_INFO("writing data to the queue for button");
          if (isEmpty == true)
            LOG_ERROR("circular buffer is full\n\r");
        }
    }
}

#else
static int32_t FLOAT_TO_INT32 (const uint8_t *buffer_ptr)
{
  uint8_t signByte = 0;
  int32_t mantissa;
  // input data format is:
  // [0]       = flags byte, bit[0] = 0 -> Celsius; =1 -> Fahrenheit
  // [3][2][1] = mantissa (2's complement)
  // [4]       = exponent (2's complement)
  // BT buffer_ptr[0] has the flags byte
  int8_t exponent = (int8_t) buffer_ptr[4];
  // sign extend the mantissa value if the mantissa is negative
  if (buffer_ptr[3] & 0x80)
    { // msb of [3] is the sign of the mantissa
      signByte = 0xFF;
    }
  mantissa = (int32_t) (buffer_ptr[1] << 0) | (buffer_ptr[2] << 8)
      | (buffer_ptr[3] << 16) | (signByte << 24);
  // value = 10^exponent * mantissa, pow() returns a double type
  return (int32_t) (pow (10, exponent) * mantissa);
} // FLOAT_TO_INT32

#endif


/*Event responder to handle the bluetooth stack activity events
 *The external event is passed as a parameter
 *No returns types
 */
void handle_ble_event (sl_bt_msg_t *evt)
{
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
        ble_data.connectionHandle = 0;
        ble_data.connect_open = false;
        ble_data.htm_indications = false;
        ble_data.inflight_indication = NOT_INFLIGHT;
        ble_data.button_state_indication=false;
        ble_data.bonding_status=0;


        sc = sl_bt_system_get_identity_address (&ble_data.myAddress,
                                                &ble_data.myAddressType);
        if (sc != SL_STATUS_OK)
          {LOG_ERROR("\n\r sl_bt_system_get_identity_address() returned != 0 status=0x%04x\n",(unsigned int) sc);
          }
        else
          {
            //displays the BT stack address on the lcd in little endian
            displayPrintf (DISPLAY_ROW_BTADDR, "%02X:%02X:%02X:%02X:%02X:%02X",
                           ble_data.myAddress.addr[0],
                           ble_data.myAddress.addr[1],
                           ble_data.myAddress.addr[2],
                           ble_data.myAddress.addr[3],
                           ble_data.myAddress.addr[4],
                           ble_data.myAddress.addr[5]);
            displayPrintf (DISPLAY_ROW_ASSIGNMENT, ASSIGNMENT); //displays assignment number
          }

        //configure sm requirements and io capability
        sc = sl_bt_sm_configure (0x0F, sl_bt_sm_io_capability_displayyesno);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("\n\r sl_bt_sm_configure() returned != 0 status=0x%04x\n",
                      (unsigned int) sc);
          }
        //delete the bondings
        sc = sl_bt_sm_delete_bondings ();
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("sl_bt_sm_delete_bondings() returned != 0 status=0x%04x",
                      (unsigned int) sc);
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
            LOG_ERROR("\n\r sl_bt_advertiser_set_timing() returned != 0 status=0x%04x\n",(unsigned int) sc);
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
        displayPrintf (DISPLAY_ROW_CONNECTION, "Advertising");
        displayPrintf (DISPLAY_ROW_9, "Button Released"); //display button state at all times


     /***************************************CLIENT************************************/
#else
        //set scanner parameters for the subsequent operations
        sc = sl_bt_scanner_set_parameters (sl_bt_scanner_scan_mode_passive,
                                           SCAN_INTERVAL, SCAN_WINDOW); //passive scanning
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("\n\r sl_bt_scanner_set_parameters() returned != 0 status=0x%04x\n",(unsigned int) sc);
          }
        //set the default bluetooth connection parameters
        sc = sl_bt_connection_set_default_parameters (MIN_INTERVAL,
                                                      MAX_INTERVAL, LATENCY,
                                                      MASTER_TIMEOUT,
                                                      MIN_CE_LEN, MAX_CE_LEN);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("\n\r sl_bt_connection_set_default_parameters() returned != 0 status=0x%04x\n",(unsigned int) sc);
          }
        //start scanning for devices
        sc = sl_bt_scanner_start (sl_bt_scanner_scan_phy_1m,
                                  sl_bt_scanner_discover_generic);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("\n\r sl_bt_scanner_start() returned != 0 status=0x%04x\n",(unsigned int) sc);
          }
        else
          {
            displayPrintf (DISPLAY_ROW_NAME, BLE_DEVICE_TYPE_STRING); // prints client on display
            displayPrintf (DISPLAY_ROW_CONNECTION, "Discovering");
          }

#endif
        break;
      }

/////////////////////////////////////CONNECTION OPENED/////////////////////////////////////////////////////////
    case sl_bt_evt_connection_opened_id:  //new connection has opened
      {
        displayPrintf (DISPLAY_ROW_CONNECTION, "Connected");
        ble_data.connectionHandle = evt->data.evt_connection_opened.connection; //connection handle

        /**********************************SERVER*********************************************/
#if DEVICE_IS_BLE_SERVER
        ble_data.connect_open = true; //connection is opened
        sc = sl_bt_advertiser_stop (ble_data.advertisingSetHandle); //stop advertising
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR( "\n\r sl_bt_advertiser_stop() returned != 0 status=0x%04x\n",(unsigned int) sc);
          }
        //request parameters
        sc = sl_bt_connection_set_parameters (ble_data.connectionHandle,
                                              MIN_INTERVAL,
                                              MAX_INTERVAL,
                                              LATENCY,
                                              MASTER_TIMEOUT,  //TIMEOUT,
                                              MIN_CE_LEN,
                                              DEFAULT_MAX_CE); //default max and min connection event length
        if (sc != SL_STATUS_OK)
          { LOG_ERROR( "\n\r sl_bt_connection_set_parameters != 0 status=0x%04x \n",(unsigned int) sc);
          }
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
    case sl_bt_evt_system_soft_timer_id: //soft timer expired
      {
        displayUpdate ();  //refresh the lcd to prevent charge buildup
        break;
      }

////////////////////////////////////////CONNECTION CLOSED//////////////////////////////////////////////////////////
    case sl_bt_evt_connection_closed_id: //connection has closed
      {
        //delete all bondings
        sc = sl_bt_sm_delete_bondings (); //empties the persistent binding database
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("sl_bt_sm_delete_bondings() returned != 0 status=0x%04x",
                      (unsigned int) sc);
          }
        //turn off the indications LEDS
        gpioLed0SetOff ();
        gpioLed1SetOff ();

        /*****************************SERVER******************************/
#if DEVICE_IS_BLE_SERVER
        ble_data.connect_open = false; //connection is closed
        ble_data.inflight_indication = false; //not inflight
        ble_data.htm_indications = false; //htm indications
        ble_data.bonding_status=0; //bonded or not
        ble_data.connectionHandle=0;
        ble_data.button_state_indication=false; //button state indications

      //generate advertising data
        sc = sl_bt_legacy_advertiser_generate_data (
            ble_data.advertisingSetHandle,
            sl_bt_advertiser_general_discoverable);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("\n\r sl_bt_legacy_advertiser_generate_data() returned != 0 status=0x%04x\n",(unsigned int) sc);
          }
        //start advertising again
        sc = sl_bt_legacy_advertiser_start (
            ble_data.advertisingSetHandle, sl_bt_legacy_advertiser_connectable);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("\n\r sl_bt_legacy_advertiser_start() returned != 0 status=0x%04x\n",(unsigned int) sc);
          }
        else{
        displayPrintf (DISPLAY_ROW_CONNECTION, "Advertising");
        displayPrintf (DISPLAY_ROW_TEMPVALUE, ""); //clear the temperature value
        displayPrintf(DISPLAY_ROW_PASSKEY, "");
        displayPrintf(DISPLAY_ROW_ACTION,"");
        }

    /*****************************CLIENT******************************/
#else
        //start scanning for devices
        sc = sl_bt_scanner_start (sl_bt_scanner_scan_phy_1m,
                                  sl_bt_scanner_discover_generic);
        if (sc != SL_STATUS_OK)
          {
            LOG_ERROR("\n\r sl_bt_scanner_start() returned != 0 status=0x%04x\n",(unsigned int) sc);
          }
        else
          {
            displayPrintf (DISPLAY_ROW_CONNECTION, "Discovering");
            displayPrintf (DISPLAY_ROW_BTADDR2, "");
            displayPrintf (DISPLAY_ROW_TEMPVALUE, "");
          }
#endif
        break;
      }

      /*****************************SERVER******************************/
#if DEVICE_IS_BLE_SERVER
/////////////////// /////////////////////CONNECTION PARAMETERS////////////////////////////////////////////
    case sl_bt_evt_connection_parameters_id: //connection parameters set by master
      {
     // LOG_INFO("\n\r MASTER SET CONNECTION PARAMETERS \n");
     //  LOG_INFO("\n\rConnection interval %d ms",
    //             (int)((evt->data.evt_connection_parameters.interval)*1.25)); //connection interval
    //   LOG_INFO("\n\rSlave latency %u ms",
    //              evt->data.evt_connection_parameters.latency); //peripheral latency
    //    LOG_INFO("\n\r Timeout %u ms",
    //            (evt->data.evt_connection_parameters.timeout)*10); //supervision timeout
        break;
      }
/////////////////////////////////////////GATT SERVER CHARACTERSITICS/////////////////////////////////////
    case sl_bt_evt_gatt_server_characteristic_status_id:
      {
        LOG_INFO("change in cccd, indications");
        /*----------------------------- TEMPERATURE MEASUREMENT-------------------------*/
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
                    gpioLed0SetOn (); //turn on LED0 when htm indications are enabled
                  } // sl_bt_gatt_server_indication
                else
                  {
                    ble_data.htm_indications = false;  //disabled htm indication
                    displayPrintf (DISPLAY_ROW_TEMPVALUE, "");
                    gpioLed0SetOff (); //turn off LED0 when htm indications are disabled
                  }
              }  //sl_bt_gatt_server_client_config

            if (evt->data.evt_gatt_server_characteristic_status.status_flags
                            == sl_bt_gatt_server_confirmation) //client acknowledged indication
                          {
                            ble_data.inflight_indication = NOT_INFLIGHT; //clear inflight flag

                            if ((ble_data.htm_indications == true)
                                && (get_queue_depth () > 0)
                                && (ble_data.bonding_status == 1))
                              {
                                isFull = read_queue (&charHandle, &bufLength, buffer);
                                if (isFull == true)
                                  LOG_ERROR("circular buffer is empty\n\r");
                                else
                                  {
                                     sc = sl_bt_gatt_server_send_indication (
                                        ble_data.connectionHandle, charHandle, bufLength,
                                        buffer);
                                    if (sc != SL_STATUS_OK)
                                      {
                                        LOG_ERROR(
                                            "sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x",
                                            (unsigned int) sc);
                                      }
                                    else
                                      ble_data.inflight_indication = true;

                                  }
                              }
                          } //sl_bt_gatt_server_confirmation

                      } //gattdb_temperature_measurement


        /*----------------------------- BUTTON PRESS -------------------------*/
        //track whether indications are enabled for the button press characteristic

             if (evt->data.evt_gatt_server_characteristic_status.characteristic
            == gattdb_button_state)
          {
            if (evt->data.evt_gatt_server_characteristic_status.status_flags
                == sl_bt_gatt_server_client_config)
              {
                if ((evt->data.evt_gatt_server_characteristic_status.client_config_flags
                    == sl_bt_gatt_server_indication)
                    || (evt->data.evt_gatt_server_characteristic_status.client_config_flags
                        == sl_bt_gatt_server_notification_and_indication))
                  {
                    ble_data.button_state_indication = true; //enabled button press indications
                    gpioLed1SetOn (); //turn on LED1 when button press indications are enabled
                    LOG_INFO("button indications enabled");
                  } // sl_bt_gatt_server_indication
                else
                  {
                    ble_data.button_state_indication = false; //disabled  button_pressed indication
                    displayPrintf (DISPLAY_ROW_TEMPVALUE, "");
                    gpioLed1SetOff (); //turn off LED0 when button press indications are disabled
                    LOG_INFO("button indications disabled");
                  }
              }  //sl_bt_gatt_server_client_config
            if (evt->data.evt_gatt_server_characteristic_status.status_flags
                == sl_bt_gatt_server_confirmation) //client acknowledged indication
              {
                LOG_INFO("client confirmation for button");
                ble_data.inflight_indication = NOT_INFLIGHT; //clear inflight flag
                if ((ble_data.button_state_indication == true) && (get_queue_depth () > 0)
                    && (ble_data.bonding_status == 1))  //if values in  queue
                  {
                    isFull = read_queue (&charHandle, &bufLength, buffer);
                    if (isFull == true)
                      LOG_ERROR("circular buffer is empty\n\r");
                    else
                      {
                         sc = sl_bt_gatt_server_send_indication (
                            ble_data.connectionHandle, charHandle, bufLength,
                            buffer);
                        if (sc != SL_STATUS_OK)
                          {
                            LOG_ERROR(
                                "sl_bt_gatt_server_send_indication() returned != 0 status=0x%04x",
                                (unsigned int) sc);
                          }
                        else
                          {
                            ble_data.inflight_indication = true;
                          LOG_INFO("sending value to client from the queue fot htm");
                          }
                      }

                  }

              } //sl_bt_gatt_server_confirmation

          } //gattdb_button_state
        break;
      }

////////////////////////////////////////////SERVER TIMEOUT///////////////////////////////////////////////////////////
    case sl_bt_evt_gatt_server_indication_timeout_id:
      {
        LOG_INFO("server timeout");
        ble_data.connect_open = false; //connection is closed
        ble_data.htm_indications = false;
        ble_data.inflight_indication = NOT_INFLIGHT; // indication not in flight
        displayPrintf (DISPLAY_ROW_CONNECTION, "");
        break;
      }

      /*****************************CLIENT********************************/
#else

//////////////////////////////////////////CLIENT SCANNED DEVICES///////////////////////////////////////////////////
    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      {
        //checking if its the matching predefined address
        if ((evt->data.evt_scanner_legacy_advertisement_report.address.addr[0]== bt_address.addr[0])&&
            (evt->data.evt_scanner_legacy_advertisement_report.address.addr[1]== bt_address.addr[1])&&
            (evt->data.evt_scanner_legacy_advertisement_report.address.addr[2]== bt_address.addr[2])&&
            (evt->data.evt_scanner_legacy_advertisement_report.address.addr[3]== bt_address.addr[3])&&
            (evt->data.evt_scanner_legacy_advertisement_report.address.addr[4]== bt_address.addr[4])&&
            (evt->data.evt_scanner_legacy_advertisement_report.address.addr[5]== bt_address.addr[5])&&
            (evt->data.evt_scanner_legacy_advertisement_report.event_flags == (SL_BT_SCANNER_EVENT_FLAG_CONNECTABLE |SL_BT_SCANNER_EVENT_FLAG_SCANNABLE)) &&             //checking if connectable and scannable
            (evt->data.evt_scanner_legacy_advertisement_report.address_type == sl_bt_gap_public_address))
          {
            sc = sl_bt_scanner_stop ();
            if (sc != SL_STATUS_OK)
              { LOG_ERROR("\n\r sl_bt_scanner_stop() returned != 0 status=0x%04x\n",(unsigned int) sc);
              }

            sc = sl_bt_connection_open ( evt->data.evt_scanner_legacy_advertisement_report.address,
                                         sl_bt_gap_public_address,
                                         sl_bt_gap_phy_1m,
                                         NULL);
            if (sc != SL_STATUS_OK)
              { LOG_ERROR( "\n\r sl_bt_connection_open() returned != 0 status=0x%04x\n",(unsigned int) sc);
              }
          }
      }
      break;
///////////////////////////////////////////////////SERVICE DISCOVERED//////////////////////////////////////////////
    case sl_bt_evt_gatt_service_id:
      {

        uint8_t *uuid = evt->data.evt_gatt_service.uuid.data;
         uint16_t uuid_length = evt->data.evt_gatt_service.uuid.len;

         if (uuid_length == 2 && memcmp(uuid, thermo_service, 2) == 0) {
             // Save the service handle of the temperature service
             ble_data.htmserviceHandle = evt->data.evt_gatt_service.service;
             LOG_INFO("htm service handle saved");
         }
          if (uuid_length == 16 && memcmp(uuid, pb_service, 16) == 0) {
             // Save the service handle of the button state service
             ble_data.pbserviceHandle = evt->data.evt_gatt_service.service;
             LOG_INFO("pb service handle saved");
         }

      }
      break;
///////////////////////////////////////////////////CHARACTERICTICS DISCOVERED////////////////////////////////////////
    case sl_bt_evt_gatt_characteristic_id:
      {
        uint8_t *uuid = evt->data.evt_gatt_characteristic.uuid.data;
        uint16_t uuid_length = evt->data.evt_gatt_characteristic.uuid.len;

                 if (uuid_length == 2 && memcmp(uuid, thermo_char, 2) == 0) {
                     //save the characteristics handle of the temperature service
                     ble_data.htmcharacteristicsHandle = evt->data.evt_gatt_characteristic.characteristic;
                     LOG_INFO("htm char handle saved");
                 }
                if (uuid_length == 16 && memcmp(uuid, pb_char, 16) == 0) {
                     //save the characteristics handle of the button state service
                     ble_data.pbcharacteristicsHandle= evt->data.evt_gatt_characteristic.characteristic;
                     LOG_INFO("pb char handle saved");
                 }
      }

      break;

      ///////////////////////////////////////////////GATT PROCEDURE COMPLETE////////////////////////////////////////////////////////////
    case sl_bt_evt_gatt_procedure_completed_id:
    {
        if (evt->data.evt_gatt_procedure_completed.result== SL_STATUS_BT_ATT_INSUFFICIENT_ENCRYPTION)
          {// encryption is required (error code 0x110F)
            sc = sl_bt_sm_increase_security(ble_data.connectionHandle); //increase security and trigger bonding process
            if (sc != SL_STATUS_OK)
              LOG_ERROR("sl_bt_sm_increase_security() returned != 0 status=0x%04x",(unsigned int) sc);
        }
    }
    break;

///////////////////////////////////////////////CHARACTERICTIC VALUE RECEIVED//////////////////////////////////////////
    case sl_bt_evt_gatt_characteristic_value_id:

      LOG_INFO("charcateristic value received");
        //htm characteristics
        if (evt->data.evt_gatt_characteristic_value.characteristic == ble_data.htmcharacteristicsHandle) //desired characteristics handle
          { //sending confirmation for the received indication from server
            sc = sl_bt_gatt_send_characteristic_confirmation (ble_data.connectionHandle);
            if (sc != SL_STATUS_OK)
                LOG_ERROR( "sl_bt_gatt_send_characteristic_confirmation()for htm returned != 0 status=0x%04x",(unsigned int) sc);
       int32_t temp_degree = FLOAT_TO_INT32 ((evt->data.evt_gatt_characteristic_value.value.data));
       displayPrintf (DISPLAY_ROW_TEMPVALUE, "Temperature = %d", temp_degree);
          } //htm characteristics

        //button state characterictics
        if (evt->data.evt_gatt_characteristic_value.characteristic == ble_data.pbcharacteristicsHandle) //desired characteristics handle
          {
            if(evt->data.evt_gatt_characteristic_value.att_opcode == sl_bt_gatt_handle_value_indication)
                            {
            //sending confirmation for the received indication from server
            sc = sl_bt_gatt_send_characteristic_confirmation (ble_data.connectionHandle);
            if (sc != SL_STATUS_OK)
                LOG_ERROR("sl_bt_gatt_send_characteristic_confirmation() for button state returned != 0 status=0x%04x",(unsigned int) sc);

              }//sl_bt_gatt_handle_value_indication

            if(evt->data.evt_gatt_characteristic_value.att_opcode == sl_bt_gatt_handle_value_indication ||
                             evt->data.evt_gatt_characteristic_value.att_opcode == sl_bt_gatt_read_response)
                            {
            if (evt->data.evt_gatt_characteristic_value.value.data[0] == 1)
              displayPrintf (DISPLAY_ROW_9, "Button Pressed");

            else
              displayPrintf (DISPLAY_ROW_9, "Button Released");
                            }
          }//button state characterictics

      break;
#endif
/********************************SERVER*&*CLIENT*EVENTS********************************************************/
      //////////////////////////////////////////BONDING REQUEST RECEIVED//////////////////////////////////////////////////////////
          case sl_bt_evt_sm_confirm_bonding_id:
            {
              LOG_INFO("bonding request received");
              sc = sl_bt_sm_bonding_confirm (ble_data.connectionHandle, 0x01); //accept the bonding request
              if (sc != SL_STATUS_OK)
                LOG_ERROR("Failed accept bonding request\n\r");
              break;
            }

      /////////////////////////////////////////////PASSKEY DISPLAY////////////////////////////////////////////////////
          case sl_bt_evt_sm_confirm_passkey_id:
            {
              LOG_INFO("passkey displayed waiting for user confirmation");
              if (ble_data.connectionHandle
                  != evt->data.evt_sm_confirm_passkey.connection)
                LOG_ERROR("Failed to confirm pass key\n\r");
              else
                {
                  displayPrintf (DISPLAY_ROW_PASSKEY, "Passkey %d",
                                 evt->data.evt_sm_confirm_passkey.passkey);
                  displayPrintf (DISPLAY_ROW_ACTION, "Confirm with PB0");

                }    //set the passkey flag and check
              break;
       }

      ///////////////////////////////////////////EXTERNAL SIGNAL RECEIVED/////////////////////////////////////////////////////////
          case sl_bt_evt_system_external_signal_id:
            {

              /*****************************SERVER********************************/
#if DEVICE_IS_BLE_SERVER
              //push button0 is pressed
              if (evt->data.evt_system_external_signal.extsignals == PUSH_BUTTON0)
                {
                  LOG_INFO("pb0 state");
                  if (ble_data.bonding_status == 0) //issue //make flag to confirm passkey (previously conifrmed)
                      sl_bt_sm_passkey_confirm (ble_data.connectionHandle, 1);

                  if ((GPIO_PinInGet (PB_port, PB0_pin) == 0)) //pressed
                    {
                      displayPrintf (DISPLAY_ROW_9, "Button Pressed");
                      if (ble_data.connect_open && ble_data.bonding_status == 1)
                        {
                          displayPrintf (DISPLAY_ROW_PASSKEY, " ");
                          displayPrintf (DISPLAY_ROW_ACTION, " ");
                          send_pushbutton_data (1); //1
                        }

                    }

                  else if ((GPIO_PinInGet (PB_port, PB0_pin) == 1)) //released
                    {
                      displayPrintf (DISPLAY_ROW_9, "Button Released");
                      if (ble_data.connect_open && ble_data.bonding_status == 1)
                        send_pushbutton_data (0); //0
                    }
                }
#else
            /*****************************CLIENT********************************/

              switch (evt->data.evt_system_external_signal.extsignals)
              {
                  case PUSH_BUTTON0:
                      if (GPIO_PinInGet(PB_port, PB0_pin) == 0) // PB0 pressed
                      {
                          LOG_INFO("pb0 state in client");
                          // Complete the pairing process
                          sl_bt_sm_passkey_confirm(ble_data.connectionHandle, 1);
                          displayPrintf(DISPLAY_ROW_PASSKEY, " ");
                          displayPrintf(DISPLAY_ROW_ACTION, " ");
                      }
                      break;

                  case PUSH_BUTTON1:
                      if ((GPIO_PinInGet(PB_port, PB1_pin) == 0) && (GPIO_PinInGet(PB_port, PB0_pin) == 1)) // PB1 pressed, PB0 released
                      {
                          LOG_INFO("pb1 state in client");
                          sc = sl_bt_gatt_read_characteristic_value(ble_data.connectionHandle, ble_data.pbcharacteristicsHandle);
                          if (sc != SL_STATUS_OK)
                              LOG_ERROR("sl_bt_gatt_read_characteristic_value() returned != 0 status=0x%04x", (unsigned int) sc);
                      }
                      // Toggle PB0 & PB1 to disable button state indications
                      else if ((GPIO_PinInGet(PB_port, PB1_pin) == 0) && (GPIO_PinInGet(PB_port, PB0_pin) == 0))
                      {
                          if (ble_data.button_state_indication) // If indications are enabled
                          {
                              sc = sl_bt_gatt_set_characteristic_notification(ble_data.connectionHandle,
                                                                              ble_data.pbcharacteristicsHandle,
                                                                              sl_bt_gatt_disable); // Disable indications
                              if (sc != SL_STATUS_OK)
                                  LOG_ERROR("sl_bt_gatt_set_characteristic_notification() returned != 0 status=0x%04x", (unsigned int) sc);
                          }
                          else // If indications are disabled
                          {
                              sc = sl_bt_gatt_set_characteristic_notification(ble_data.connectionHandle,
                                                                              ble_data.pbcharacteristicsHandle,
                                                                              sl_bt_gatt_indication); // Enable indications
                              if (sc != SL_STATUS_OK)
                                  LOG_ERROR("sl_bt_gatt_set_characteristic_notification() returned != 0 status=0x%04x", (unsigned int) sc);
                          }
                      }
                      break;

                  default:
                      LOG_INFO("Unhandled button press event");
                      break;
              }
/*//push button0 is pressed
            if (evt->data.evt_system_external_signal.extsignals == PUSH_BUTTON0)
              {
                if ((GPIO_PinInGet (PB_port, PB0_pin) == 0)) //pb0 pressed
                  {
                LOG_INFO("pb0 state in client");
                  //complete the pairing process
                    sl_bt_sm_passkey_confirm (ble_data.connectionHandle, 1);
                    displayPrintf (DISPLAY_ROW_PASSKEY, " ");
                    displayPrintf (DISPLAY_ROW_ACTION, " ");
                  }
              }

            //push button 1 pressed
            else if (evt->data.evt_system_external_signal.extsignals == PUSH_BUTTON1)
              {
                          //read request given to client by pressing PB1
                if ((GPIO_PinInGet (PB_port, PB1_pin) == 0)&& (GPIO_PinInGet (PB_port, PB0_pin) == 1)) //pb1 pressed and pb0 released
                {
                    LOG_INFO("pb1 state in client");
                sc=sl_bt_gatt_read_characteristic_value(ble_data.connectionHandle,ble_data.pbcharacteristicsHandle);
                if (sc != SL_STATUS_OK)
                         LOG_ERROR( "sl_bt_gatt_read_characteristic_value() returned != 0 status=0x%04x",(unsigned int) sc);

                }

                 //toggle the pb0 and pb1 to disable button state indications
            else if ((GPIO_PinInGet (PB_port, PB1_pin) == 0)&&(GPIO_PinInGet (PB_port, PB0_pin) == 0))
              {
                if (ble_data.button_state_indication) //if button indications are enabled
                  {
                    sc = sl_bt_gatt_set_characteristic_notification (ble_data.connectionHandle,
                                                                     ble_data.pbcharacteristicsHandle,
                                                                     sl_bt_gatt_disable); //disable indications
                    if (sc != SL_STATUS_OK)
                       LOG_ERROR(  "sl_bt_gatt_set_characteristic_notification() returned != 0 status=0x%04x",(unsigned int) sc);
                    //ble_data.button_state_indication = false;
                  }
                else  //if disabled
                  {
                    sc = sl_bt_gatt_set_characteristic_notification ( ble_data.connectionHandle,
                                                                      ble_data.pbcharacteristicsHandle,
                                                                      sl_bt_gatt_indication); //enable indications
                    if (sc != SL_STATUS_OK)

                        LOG_ERROR(  "sl_bt_gatt_set_characteristic_notification() returned != 0 status=0x%04x",(unsigned int) sc);
                 // ble_data.button_state_indication = true;
                  }
              }

              }*/

#endif

              break;
            }


      //////////////////////////////////////////////////BONDING SUCCESSFULL//////////////////////////////////////////////////
          case sl_bt_evt_sm_bonded_id:
            {
              LOG_INFO("bonding successfull");
              ble_data.bonding_status = 1;     //saving bonding status
              displayPrintf (DISPLAY_ROW_CONNECTION, "Bonded");
              displayPrintf (DISPLAY_ROW_PASSKEY, " ");
              displayPrintf (DISPLAY_ROW_ACTION, " ");
              break;
            }

      ///////////////////////////////////////////////BONDING FAILED/////////////////////////////////////////////////////////////
          case sl_bt_evt_sm_bonding_failed_id:
            {
              LOG_INFO("bonding failed");
              ble_data.bonding_status = 0;
              displayPrintf (DISPLAY_ROW_CONNECTION, "Bonding Failed");
              LOG_ERROR("bonding failed");
              break;
            }


    default:
      break; //break
    } // end - switch
}

