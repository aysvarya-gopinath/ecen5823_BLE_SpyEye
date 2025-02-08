/*
 timers.c
 This file includes configuration of LETIMER for periodic interrupts at different power modes
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

#include "em_letimer.h"
#include "timers.h"
#include "em_cmu.h"
#include"app.h"
#include "em_gpio.h"
// Include logging for this file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

void initLETIMER0 ()
{
  uint32_t temp; // this data structure is passed to LETIMER_Init (), used to set LETIMER0_CTRL reg bits and other values
  const LETIMER_Init_TypeDef letimerInitData =
    {
    false, // enable;   don't enable when init completes, we'll enable last
        true, // debugRun; useful to have the timer running when single-stepping in the debugger
        true,   // comp0Top; load COMP0 into CNT on underflow
        false,   // bufTop;   don't load COMP1 into COMP0 when REP0==0
        0,        // out0Pol;  0 default output pin value
        0,          // out1Pol;  0 default output pin value
        letimerUFOANone,   // ufoa0;    no underflow output action
        letimerUFOANone,    // ufoa1;    no underflow output action
        letimerRepeatFree, // repMode;  free running mode i.e. load & go forever
        0                 // COMP0(top) Value, I calculate this below
      };

  LETIMER_Init (LETIMER0, &letimerInitData);// init the timer
 //ulfrco
  LETIMER_CompareSet(LETIMER0,0,COMP0_LOAD);// load COMP0 (top)


// Clear all IRQ flags in the LETIMER0 IF status register
  LETIMER_IntClear (LETIMER0, 0xFFFFFFFF); // punch them all down

// Set UF  in LETIMER0_IEN, so that the timer will generate IRQs to the NVIC.
  temp = LETIMER_IEN_UF;

  LETIMER_IntEnable (LETIMER0, temp); // Make sure you have defined the ISR routine LETIMER0_IRQHandler()

// Enable the timer to starting counting down, set LETIMER0_CMD[START] bit, see LETIMER0_STATUS[RUNNING] bit
  LETIMER_Enable (LETIMER0, true);

// read it a few times to make sure it's running within the range of values we expect
  temp = LETIMER_CounterGet (LETIMER0);
  temp = LETIMER_CounterGet (LETIMER0);
  temp = LETIMER_CounterGet (LETIMER0);
  //uint32_t freq=CMU_ClockFreqGet(cmuClock_LETIMER0);
    // LOG_INFO("frequency %u",freq);


} // initLETIMER0()

//waits in micro seconds
void timerWaitUs (uint32_t us_wait)
{
  uint32_t current_ticks, max_load, wait_cnt, wrap_cnt; // all time values with respect to ticks

if ((us_wait<=MAX_WAIT)&&(us_wait>=MIN_WAIT))
  { // in the range
//ulfrco
 // wait_cnt = ((us_wait/1000)* ULFRCO_FREQ)/1000;  //old
    wait_cnt = (us_wait* ULFRCO_FREQ)/US_PER_S; // truncation
  max_load = COMP0_LOAD; // maximum value of the ticks
  current_ticks = LETIMER_CounterGet(LETIMER0); //holds the current CNT value
  //case 1
//old if (wait_cnt < current_ticks) //wait time is less than the current time
  if ((wait_cnt <= max_load) && (wait_cnt <= current_ticks)) //wait time is less than the current time
    {
      while (LETIMER_CounterGet(LETIMER0)> (current_ticks-wait_cnt))
          {} //
    }// wait_cnt is within max_load and current_ticks

  //case 2
  else //wait time is greater than the current time
    {
  // wrap_cnt=(max_load-wait_cnt)+current_ticks; old
      while (!(LETIMER_IntGet(LETIMER0) & LETIMER_IF_UF)) { //new
      }  // Busy wait for underflow
      wrap_cnt = wait_cnt - current_ticks; //the additional time to wait
   while (LETIMER_CounterGet (LETIMER0) !=wrap_cnt) //wrapping up
               {}
    } //wait_cnt is greater than current ticks but less than max load

}//in the range

 else //wait time is out of range
   {
LOG_ERROR("\n\r OUT OF RANGE,Invalid wait time: %u\n",us_wait);//log error

   } // wait time is more than the max load cnt

}

void unit_test_timerWaitUs() {
   // LOG_INFO("Running unit tests for TimerWaitUs()...\n");

  // Negative test (should not do anything)
      //LOG_INFO("Testing negative value (should not delay)\n");
      GPIO_PinOutToggle(gpioPortF,5);
      timerWaitUs(-10000);
      GPIO_PinOutToggle(gpioPortF,5);

      // Small delay test (10ms)
    GPIO_PinOutToggle(gpioPortF,5);
    timerWaitUs(10000);  // 10ms
    GPIO_PinOutToggle(gpioPortF,5);

    // Medium delay test (100ms)
    GPIO_PinOutToggle(gpioPortF,5);
    timerWaitUs(100000);  // 100ms
    GPIO_PinOutToggle(gpioPortF,5);

    // Large delay test (1s)
    GPIO_PinOutToggle(gpioPortF,5);
    timerWaitUs(1000000);  // 1s
    GPIO_PinOutToggle(gpioPortF,5);

    // Out-of-range test (4s - should fail)
   // LOG_INFO("Testing out-of-range value (should not delay)\n");
    GPIO_PinOutToggle(gpioPortF,5);
    timerWaitUs(4000000);
    GPIO_PinOutToggle(gpioPortF,5);



   // LOG_INFO("All tests completed. Observe LED blinking and Energy Profiler output.\n");
}

