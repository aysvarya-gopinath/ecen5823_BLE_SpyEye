/*
  timers.h

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

#ifndef __myTimers
#define __myTimers

#define LETIMER_ON_TIME_MS  (175) //led on-time
#define LETIMER_PERIOD_MS   (2250)
// ULFRCO
#define ULFRCO_FREQ         (1000)   //1000hz
#define COMP1_LOAD          (ULFRCO_FREQ*LETIMER_ON_TIME_MS/1000   //175
#define COMP0_LOAD          (ULFRCO_FREQ*LETIMER_PERIOD_MS)/1000    //2250
//LFXO
#define PRESCALER_VALUE     (4)
#define LFXO_CLK_FREQ       (32768/PRESCALER_VALUE)          //8192 Hz for LFXO
#define COMP0_VAL           (LETIMER_PERIOD_MS*LFXO_CLK_FREQ)/1000  //18,432
#define COMP1_VAL           (LETIMER_ON_TIME_MS*LFXO_CLK_FREQ)/1000 // 1433 on time

/**************************************************************************//**
 * Initialization LETIMER0
 *****************************************************************************/
void initLETIMER0 ();






#endif
