/*
 * ble.h
 *This file contains the BLE implementation functions
 *  Created on: Feb 17, 2025
 *  Author:@Aysvarya Gopinath   Aysvarya.Gopinath@Colorado.edu
 */

#ifndef SRC_BLE_H_
#define SRC_BLE_H_
#include "sl_bgapi.h"
#include"sl_bt_api.h"


//Set the Advertising minimum and maximum to 250mS
#define ADVERTISING_MIN   400 //Value in units of 0.625 ms
#define ADVERTISING_MAX  400  //250ms /0.625=400
// Set Connection Interval minimum and maximum to 75mS
#define MIN_INTERVAL 60 // 75ms/1.25 =60(value)
#define MAX_INTERVAL 60 //
// Set the Slave latency to enable it to be “off the air” for up to 300mS
#define  LATENCY 4 // 300ms/75ms
// Set the Supervision timeout to a value greater than (1 + slave latency) *(connection_interval * 2)
#define TIMEOUT (80) //80       //((1 + 4) * (75*2)/10)
#define MASTER_TIMEOUT (825)  //(1+4)*(75*2)+75
#define MIN_CE_LEN 0
#define DEFAULT_MAX_CE (0xffff)
#define MAX_CE_LEN 4  //master
#define MAX_EVENTS 0
#define DURATION 0
#define SCAN_WINDOW 40 //(25ms/0.625ms)
#define SCAN_INTERVAL  80 //(50ms/0.625ms)
#define UINT8_TO_BITSTREAM(p, n)      { *(p)++ = (uint8_t)(n); }
#define UINT32_TO_BITSTREAM(p, n)     { *(p)++ = (uint8_t)(n); *(p)++ = (uint8_t)((n) >> 8); \
                                        *(p)++ = (uint8_t)((n) >> 16); *(p)++ = (uint8_t)((n) >> 24); }
#define INT32_TO_FLOAT(m, e)         ( (int32_t) (((uint32_t) m) & 0x00FFFFFFU) | (((uint32_t) e) << 24) )
#define NOT_INFLIGHT (0)
#define INFLIGHT (1)

// BLE Data Structure, save all of our private BT data in here.
// Modern C (circa 2021 does it this way)
// typedef ble_data_struct_t is referred to as an anonymous struct definition
typedef struct {
// values that are common to servers and clients
bd_addr  myAddress; //BT device address
uint8_t  advertisingSetHandle; //to store the handle
uint8_t myAddressType; //identity address type
bool connect_open; //flag to track if connection is opened(true) or closed(false)
int inflight_indication; //indication status inflight (1) or not (0)(server sends indications to client)
bool htm_indications; //true when htm indications are enabled (client enables/disables notifications/indications from server)
uint32_t connectionHandle; //connection handle
uint32_t serviceHandle; //service handle
uint16_t characteristicsHandle; //characteristics handle
} ble_data_struct_t;


//function to return a pointer to the ble_data structure
ble_data_struct_t* get_ble_dataPtr(void);

//function to handle the BT stack events
void handle_ble_event(sl_bt_msg_t *evt);

//function to send the temperature to the gatt db and app
void send_temp_ble(int32_t temp_deg);

#endif /* SRC_BLE_H_ */

