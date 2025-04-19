/*
gpio.h

   Created on: Dec 12, 2018
       Author: Dan Walkes
   Updated by Dave Sluiter Dec 31, 2020. Minor edits with #defines.

   March 17
   Dave Sluiter: Use this file to define functions that set up or control GPIOs.

   Jan 24, 2023
   Dave Sluiter: Cleaned up gpioInit() to make it less confusing for students regarding
                 drive strength setting.

 *
 * @Aysvarya Gopinath   Aysvarya.Gopinath@Colorado.edu
 *
 */


// Students: Remember, a header file (a .h file) generally defines an interface
//           for functions defined within an implementation file (a .c file).
//           The .h file defines what a caller (a user) of a .c file requires.
//           At a minimum, the .h file should define the publicly callable
//           functions, i.e. define the function prototypes. #define and type
//           definitions can be added if the caller requires theses.


#ifndef SRC_GPIO_H_
#define SRC_GPIO_H_
#include "em_gpio.h"

// Student Edit: Define these, 0's are placeholder values.
//
// See the radio board user guide at https://www.silabs.com/documents/login/user-guides/ug279-brd4104a-user-guide.pdf
// and GPIO documentation at https://siliconlabs.github.io/Gecko_SDK_Doc/efm32g/html/group__GPIO.html
// to determine the correct values for these.
// If these links have gone bad, consult the reference manual and/or the datasheet for the MCU.
// Change to correct port and pins:
#define LED_port   (gpioPortF)
#define LED0_pin   (4) //PF4
#define LED1_pin   (5)  //PF5

#define SI7021_port  (gpioPortD) //sensor and EXTCOMIN pin has the same port
#define SI7021_pin    15 //PD15
#define EXT_COMIN_pin   13 //PD13

#define PB_port  (gpioPortF)
#define PB0_pin (6)  //PF6
#define PB1_pin (7) //PF7

//expansion pin 3
#define PA_port (gpioPortA)
#define PIR_sensor_pin (2) //PA2


// Function prototypes
void gpioInit();
void gpioLed0SetOn();
void gpioLed0SetOff();
void gpioLed1SetOn();
void gpioLed1SetOff();

//sensor on/off functions
void gpioSi7021ON();
void gpioSi7021OFF();

// set/ clear the extcomin pin
void gpioSetDisplayExtcomin(bool extcomin_state);

#endif /* SRC_GPIO_H_ */
