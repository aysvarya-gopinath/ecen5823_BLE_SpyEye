/*
 gpio.c
 
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

// *****************************************************************************
// Students:
// We will be creating additional functions that configure and manipulate GPIOs.
// For any new GPIO function you create, place that function in this file.
// *****************************************************************************
#include <stdbool.h>

#include <string.h>
#include "gpio.h"
#include"lcd.h"

// Set GPIO drive strengths and modes of operation
void gpioInit ()
{

  // Set the port's drive strength. In this MCU implementation, all GPIO cells
  // in a "Port" share the same drive strength setting.
  //GPIO_DriveStrengthSet(LED_port, gpioDriveStrengthStrongAlternateStrong); // Strong, 10mA
  GPIO_DriveStrengthSet (LED_port, gpioDriveStrengthWeakAlternateWeak); // Weak, 1mA

  // Set the 2 GPIOs mode of operation
  GPIO_PinModeSet (LED_port, LED0_pin, gpioModePushPull, false);
  GPIO_PinModeSet (LED_port, LED1_pin, gpioModePushPull, false);

  //configure sensor and lcd pins
  GPIO_PinModeSet (SI7021_port, SI7021_pin, gpioModePushPull, false);
  GPIO_PinModeSet (SI7021_port, EXT_COMIN_pin, gpioModePushPull, true);

  //configure the push button 0
  GPIO_PinModeSet (PB_port, PB0_pin,gpioModeInputPullFilter,true);

  //configure the gpio external pin interrupt
  GPIO_ExtIntConfig (PB_port, PB0_pin, PB0_pin, true, true, true);

  //configure the push button 1
   GPIO_PinModeSet (PB_port, PB1_pin,gpioModeInputPullFilter,true);

   //configure the gpio external pin interrupt
   GPIO_ExtIntConfig (PB_port, PB1_pin, PB1_pin, true, true, true);

   //set the pir sensor pin as input
   GPIO_PinModeSet(PA_port, PIR_sensor_pin, gpioModeInputPull, 0);

  // GPIO_PinModeSet (PA_port, PIR_sensor_pin, gpioModeInput, true);
   //configure the pir sensor pin interrupt
 //  GPIO_ExtIntConfig (PA_port, PIR_sensor_pin,PIR_sensor_pin, true, true, true);


} // gpioInit()

/*Function to turn on the temperature sensor
 * No parameters no return types
 */
void gpioSi7021ON()
{
  GPIO_PinOutSet( SI7021_port, SI7021_pin );
}

/*Function to turn off the temperature sensor
 * No parameters no return types
 */
void gpioSi7021OFF()
{
  GPIO_PinOutClear( SI7021_port, SI7021_pin );
}

/*Function to turn on the LED0
 * No parameters no return types
 */
void gpioLed0SetOn ()
{
  GPIO_PinOutSet (LED_port, LED0_pin);
}

/*Function to turn off the LED0
 * No parameters no return types
 */
void gpioLed0SetOff ()
{
  GPIO_PinOutClear (LED_port, LED0_pin);
}

/*Function to turn on the LED1
 * No parameters no return types
 */
void gpioLed1SetOn ()
{
  GPIO_PinOutSet (LED_port, LED1_pin);
}

/*Function to turn off the LED1
 * No parameters no return types
 */
void gpioLed1SetOff ()
{
  GPIO_PinOutClear (LED_port, LED1_pin);
}

/*Function to set / clear the extcomin pin to handle polarity inversion
 * Parameter- Previous state of the Extcomin pin
 * NO return types
 */
void gpioSetDisplayExtcomin(bool extcomin_state)
{
  if (extcomin_state)
    GPIO_PinOutSet (SI7021_port, EXT_COMIN_pin); //set the pin to control polarity inversion
  else
    GPIO_PinOutClear (SI7021_port, EXT_COMIN_pin); //clear the pin
}
