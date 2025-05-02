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
#define INCLUDE_LOG_DEBUG 0 //
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
bool intruder_alert_active=true;
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



/*This function sends motion activity data over BLE
 * No return types
 *1(when intruder found) and 0 (when no activity)are the  parameters
 */

void send_motion_ble(uint8_t message){
  sl_status_t sc;
//write activity to gatt db
  sc = sl_bt_gatt_server_write_attribute_value (
      gattdb_alert_message, // handle from gatt_db.h
      0,             // offset
      1,            // length
      &message     // in IEEE-11073 format
      );

  if (sc != SL_STATUS_OK)
    LOG_ERROR("\n\r sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x\n", (unsigned int) sc);

  if ((ble_data.motion_indications == true)&&(ble_data.bonding_status == 1))//if the indications are enabled
    {
      if (ble_data.inflight_indication == false && (get_queue_depth () == 0)) //if not inflight and queue is empty
        {
          sc = sl_bt_gatt_server_send_indication (ble_data.connectionHandle,
                                                  gattdb_alert_message, // handle from gatt_db.h
                                                  1,
                                                  &message // in IEEE-11073 format
            );
          if (sc != SL_STATUS_OK)
              LOG_ERROR("\n\r sl_bt_gatt_server_send_indication() for temp returned != 0 status=0x%04x\n", (unsigned int) sc);
          else
              ble_data.inflight_indication = INFLIGHT; //mark as in transit
        }
      else           //if queue is not empty and inflight
          write_queue ( gattdb_alert_message, 5,(uint8_t*)&message);
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
      LOG_ERROR( "\n\r sl_bt_gatt_server_write_attribute_value() returned != 0 status=0x%04x\n",(unsigned int) sc);

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
            LOG_ERROR("\n\r sl_bt_gatt_server_send_indication() for button returned != 0 status=0x%04x\n",(unsigned int) sc);
          else
             ble_data.inflight_indication = true; // indications are inflight
        }
      else
          isEmpty = write_queue (gattdb_button_state, sizeof(uint8_t),(uint8_t*) &pb_data);
    }
}


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
        ble_data.motion_indications = false;
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
        LOG_INFO("\n\rAdvertising \n");
        displayPrintf (DISPLAY_ROW_NAME, BLE_DEVICE_TYPE_STRING); // prints Server on display
        displayPrintf (DISPLAY_ROW_CONNECTION, "Advertising");
        displayPrintf (DISPLAY_ROW_9, "Button Released"); //display button state at all times

        break;
                   }
/////////////////////////////////////CONNECTION OPENED/////////////////////////////////////////////////////////
    case sl_bt_evt_connection_opened_id:  //new connection has opened
      {
         LOG_INFO("\n\r Connected \n");
        displayPrintf (DISPLAY_ROW_CONNECTION, "Connected");
        ble_data.connectionHandle = evt->data.evt_connection_opened.connection; //connection handle

        /**********************************SERVER*********************************************/

        ble_data.connect_open = true; //connection is opened
        sc = sl_bt_advertiser_stop (ble_data.advertisingSetHandle); //stop advertising
        if (sc != SL_STATUS_OK)
            LOG_ERROR( "\n\r sl_bt_advertiser_stop() returned != 0 status=0x%04x\n",(unsigned int) sc);

        //request parameters
        sc = sl_bt_connection_set_parameters (ble_data.connectionHandle,
                                              MIN_INTERVAL,
                                              MAX_INTERVAL,
                                              LATENCY,
                                              MASTER_TIMEOUT,  //TIMEOUT,
                                              MIN_CE_LEN,
                                              DEFAULT_MAX_CE); //default max and min connection event length
        if (sc != SL_STATUS_OK)
           LOG_ERROR( "\n\r sl_bt_connection_set_parameters != 0 status=0x%04x \n",(unsigned int) sc);

        break;
           }

///////////////////////////////////////LAZY SOFT TIMER EXPIRY////////////////////////////////////////////////////////
    case sl_bt_evt_system_soft_timer_id: //soft timer expired
      {
        displayUpdate ();  //refresh the lcd to prevent charge buildup
        break;
      }


      ///////////////////////////////////////CLIENT WRITE WITHOUT ACK////////////////////////////////////////////////////////
    case sl_bt_evt_gatt_server_attribute_value_id:
    {
        uint16_t handle = evt->data.evt_gatt_server_attribute_value.attribute;
        const uint8_t *value = evt->data.evt_gatt_server_attribute_value.value.data;
        size_t len = evt->data.evt_gatt_server_attribute_value.value.len;

        if (handle == gattdb_alert_message && len == 1 && value[0] == '1') {
            intruder_alert_active = false;
        }
    }
    break;


////////////////////////////////////////CONNECTION CLOSED//////////////////////////////////////////////////////////
    case sl_bt_evt_connection_closed_id: //connection has closed
      {
        //delete all bondings
        sc = sl_bt_sm_delete_bondings (); //empties the persistent binding database
        if (sc != SL_STATUS_OK)
          LOG_ERROR("sl_bt_sm_delete_bondings() returned != 0 status=0x%04x",(unsigned int) sc);

        ble_data.bonding_status=0;//bonded or not
        ble_data.connectionHandle=0;
        ble_data.button_state_indication=false; //button state indications

        /*****************************SERVER******************************/

        ble_data.connect_open = false; //connection is closed
        ble_data.inflight_indication = false; //not inflight
        ble_data.motion_indications = false; //motion indications

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
        displayPrintf (DISPLAY_ROW_TEMPVALUE, ""); //clear the activity value
        displayPrintf(DISPLAY_ROW_PASSKEY, "");
        displayPrintf(DISPLAY_ROW_ACTION,"");
        }
      }
       break;
      /*****************************SERVER******************************/

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
        /*----------------------------- TEMPERATURE MEASUREMENT-------------------------*/
        if (evt->data.evt_gatt_server_characteristic_status.characteristic
            == gattdb_alert_message)
          {
            if (evt->data.evt_gatt_server_characteristic_status.status_flags
                == sl_bt_gatt_server_client_config)
              {
                if ((evt->data.evt_gatt_server_characteristic_status.client_config_flags
                    == sl_bt_gatt_server_indication)
                    || (evt->data.evt_gatt_server_characteristic_status.client_config_flags
                        == sl_bt_gatt_server_notification_and_indication))
                  {
                    ble_data.motion_indications = true;  //enabled motion indications
                  } // sl_bt_gatt_server_indication
                else
                  {
                    ble_data.motion_indications = false;  //disabled motion indication
                    displayPrintf (DISPLAY_ROW_TEMPVALUE, "");
                  }
              }  //sl_bt_gatt_server_client_config

            if (evt->data.evt_gatt_server_characteristic_status.status_flags
                == sl_bt_gatt_server_confirmation) //client acknowledged indication
              {
                ble_data.inflight_indication = NOT_INFLIGHT; //clear inflight flag
                isFull = read_queue (&charHandle, &bufLength, buffer);
                if (isFull != true){
                        sc = sl_bt_gatt_server_send_indication (
                        ble_data.connectionHandle, charHandle, bufLength,
                        buffer);
                    if (sc != SL_STATUS_OK)
                      LOG_ERROR( "sl_bt_gatt_server_send_indication()while dequeue in temp returned != 0 status=0x%04x",(unsigned int) sc);

                    else
                      ble_data.inflight_indication = true;

                  }
              } //sl_bt_gatt_server_confirmation
          } //gattdb_activity_measurement


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
                  } // sl_bt_gatt_server_indication
                else
                  {
                    ble_data.button_state_indication = false; //disabled  button_state indication
                  }
              }  //sl_bt_gatt_server_client_config

            if (evt->data.evt_gatt_server_characteristic_status.status_flags
                == sl_bt_gatt_server_confirmation) //client acknowledged indication
              {
                ble_data.inflight_indication = NOT_INFLIGHT; //clear inflight flag
                isFull= read_queue (&charHandle, &bufLength, buffer);
                    if (isFull != true){
                   sc = sl_bt_gatt_server_send_indication (
                            ble_data.connectionHandle, charHandle, bufLength,
                            buffer);
                        if (sc != SL_STATUS_OK)
                            LOG_ERROR( "sl_bt_gatt_server_send_indication() while dequeue in button returned != 0 status=0x%04x", (unsigned int) sc);

                        else
                            ble_data.inflight_indication = true;
                    }
              } //sl_bt_gatt_server_confirmation

          } //gattdb_button_state
        break;
      }

////////////////////////////////////////////SERVER TIMEOUT///////////////////////////////////////////////////////////
    case sl_bt_evt_gatt_server_indication_timeout_id:
      {
        ble_data.connect_open = false; //connection is closed
        ble_data.motion_indications = false;
        ble_data.inflight_indication = NOT_INFLIGHT; // indication not in flight
        displayPrintf (DISPLAY_ROW_CONNECTION, "");
        break;
      }

/********************************SERVER*&*CLIENT*EVENTS********************************************************/
      //////////////////////////////////////////BONDING REQUEST RECEIVED//////////////////////////////////////////////////////////
    case sl_bt_evt_sm_confirm_bonding_id:
      {
        sc = sl_bt_sm_bonding_confirm (ble_data.connectionHandle, 0x01); //accept the bonding request
        if (sc != SL_STATUS_OK)
          LOG_ERROR("Failed accept bonding request\n\r");
        break;
      }

      /////////////////////////////////////////////PASSKEY DISPLAY////////////////////////////////////////////////////
    case sl_bt_evt_sm_confirm_passkey_id:
      {

        if (ble_data.connectionHandle != evt->data.evt_sm_confirm_passkey.connection)
          LOG_ERROR("Failed to confirm pass key\n\r");
        else
          {
            displayPrintf (DISPLAY_ROW_PASSKEY, "Passkey %d",evt->data.evt_sm_confirm_passkey.passkey);
            displayPrintf (DISPLAY_ROW_ACTION, "Confirm with PB0");

          }    //set the passkey flag and check
        break;
      }

      ///////////////////////////////////////////EXTERNAL SIGNAL RECEIVED/////////////////////////////////////////////////////////
          case sl_bt_evt_system_external_signal_id:
            {

              /*****************************SERVER********************************/


        //push button0 is pressed
        if (evt->data.evt_system_external_signal.extsignals == PB0_press)
          {

            if (ble_data.bonding_status == 0) //issue //make flag to confirm passkey (previously conifrmed)
              sl_bt_sm_passkey_confirm (ble_data.connectionHandle, 1);
            if (ble_data.connect_open && ble_data.bonding_status == 1)
                send_pushbutton_data (1);
            displayPrintf (DISPLAY_ROW_PASSKEY, " ");
            displayPrintf (DISPLAY_ROW_ACTION, " ");
            displayPrintf (DISPLAY_ROW_9, "Button Pressed");
          }
        if (evt->data.evt_system_external_signal.extsignals == PB0_release) //released
          {
            if (ble_data.connect_open && ble_data.bonding_status == 1)
              send_pushbutton_data (0);
            displayPrintf (DISPLAY_ROW_9, "Button Released");
          }

        break;
      }

      //////////////////////////////////////////////////BONDING SUCCESSFULL//////////////////////////////////////////////////
    case sl_bt_evt_sm_bonded_id:
      {
        ble_data.bonding_status = 1;     //saving bonding status
        displayPrintf (DISPLAY_ROW_CONNECTION, "Bonded");
        displayPrintf (DISPLAY_ROW_PASSKEY, " ");
        displayPrintf (DISPLAY_ROW_ACTION, " ");
        break;
      }

      ///////////////////////////////////////////////BONDING FAILED/////////////////////////////////////////////////////////////
    case sl_bt_evt_sm_bonding_failed_id:
      {
        ble_data.bonding_status = 0;
        displayPrintf (DISPLAY_ROW_CONNECTION, "Bonding Failed");
        LOG_ERROR("bonding failed");
        break;
      }

    default:
      break; //break
    } // end - switch
}

