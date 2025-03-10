/***********************************************************************
 * @file      lcd.h
 * @version   1.0
 * @brief     LCD header file. A complete re-write of the LCD support code
 *            based on Gecko SDK 3.1 and Simplicity Studio 5.1.
 *
 * @author    Dave Sluiter, David.Sluiter@colorado.edu
 * @date      March 15, 2021
 *
 * @institution University of Colorado Boulder (UCB)
 * @course      ECEN 5823: IoT Embedded Firmware
 * @instructor  David Sluiter
 *
 * @assignment Starter code
 * @due        NA
 *
 * @resources  This code is based on the Silicon Labs example MEMLCD_baremetal
 *             as part of SSv5 and Gecko SDK 3.1.
 *
 * @copyright  All rights reserved. Distribution allowed only for the
 * use of assignment grading. Use of code excerpts allowed at the
 * discretion of author. Contact for permission.
 *
 * Editor: Feb 26, 2022, Dave Sluiter
 * Change: Added comment about use of .h files.
 *
 */

// Students: Remember, a header file (a .h file) generally defines an interface
//           for functions defined within an implementation file (a .c file).
//           The .h file defines what a caller (a user) of a .c file requires.
//           At a minimum, the .h file should define the publicly callable
//           functions, i.e. define the function prototypes. #define and type
//           definitions can be added if the caller requires theses.


#ifndef SRC_LCD_H_
#define SRC_LCD_H_


#define SOFT_TIME (32768) //ticks for the soft timer operating at 1Hz delay
#define SINGLE_SHOT (0) //false -timer is repetitive
#define SLACK (10) //additional delay
#define HANDLE (0) // only 1 soft timer handle can be 0


/**
 * Display row definitions, used for writing specific content based on
 * assignment requirements. See assignment text for details.
 */
enum display_row {
	DISPLAY_ROW_NAME,          // 0
	DISPLAY_ROW_BTADDR,        // 1
	DISPLAY_ROW_BTADDR2,       // 2
	DISPLAY_ROW_CLIENTADDR,    // 3
	DISPLAY_ROW_CONNECTION,    // 4
	DISPLAY_ROW_PASSKEY,       // 5
	DISPLAY_ROW_ACTION,        // 6
	DISPLAY_ROW_TEMPVALUE,     // 7
	DISPLAY_ROW_8,             // 8
	DISPLAY_ROW_9,             // 9
	DISPLAY_ROW_10,            // 10
	DISPLAY_ROW_11,            // 11
	DISPLAY_ROW_ASSIGNMENT,    // 12
	DISPLAY_NUMBER_OF_ROWS     // 13
};

// The number of characters per row
#define DISPLAY_ROW_LEN      20
#define ASSIGNMENT ("A7")


// function prototypes

void displayInit();
void displayUpdate();
void displayPrintf(enum display_row row, const char *format, ...);




#endif /* SRC_LCD_H_ */
