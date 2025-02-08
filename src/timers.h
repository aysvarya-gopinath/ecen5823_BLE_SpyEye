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

#define LETIMER_PERIOD_MS   (3000) //3000ms is 3sec
#define US_PER_S           (1000000) //micro seconds in 1second
#define MIN_WAIT            (1000)      //minimum wait is 1000us and 1ms
#define MAX_WAIT             (3000000)   // maximum wait is 3000ms
// ULFRCO
#define ULFRCO_FREQ         (1000)   //1000hz
#define COMP0_LOAD          (ULFRCO_FREQ*LETIMER_PERIOD_MS)/1000 //3000

/**************************************************************************//**
 * Initialization LETIMER0
 *****************************************************************************/
void initLETIMER0 (void);

/**************************************************************************//**
 *  LETIMER0 wait
 *****************************************************************************/
void timerWaitUs(uint32_t us_wait);


/**************************************************************************//**
 *  LETIMER0 unit test
 *****************************************************************************/
void unit_test_timerWaitUs();


#endif
