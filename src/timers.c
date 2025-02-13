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

  LETIMER_Init (LETIMER0, &letimerInitData);                 // init the timer
  LETIMER_CompareSet (LETIMER0, 0, COMP0_LOAD); // load COMP0 (top)
  LETIMER_IntClear (LETIMER0, 0xFFFFFFFF); // Clear all IRQ flags in the LETIMER0 IF status register
  temp = LETIMER_IEN_UF; // Set UF  in LETIMER0_IEN, so that the timer will generate IRQs to the NVIC.
  LETIMER_IntEnable (LETIMER0, temp); // Make sure you have defined the ISR routine LETIMER0_IRQHandler()
  LETIMER_Enable (LETIMER0, true); // Enable the timer to starting counting down, set LETIMER0_CMD[START] bit, see LETIMER0_STATUS[RUNNING] bit
  temp = LETIMER_CounterGet (LETIMER0); // read it a few times to make sure it's running within the range of values we expect
  temp = LETIMER_CounterGet (LETIMER0);
  temp = LETIMER_CounterGet (LETIMER0);

}

//timer wait by interrupts in micro-seconds
void timerWaitUs_irq(uint32_t us_wait)
{
  uint32_t current_ticks, max_load, wait_cnt, wrap_cnt; // all time values with respect to ticks
  if ((us_wait <= MAX_WAIT) && (us_wait >= MIN_WAIT)) // in the range
    {
      wait_cnt = (us_wait * ULFRCO_FREQ) / US_PER_S; // truncation
      max_load = COMP0_LOAD; // maximum value of the ticks for ulfrco
      current_ticks = LETIMER_CounterGet (LETIMER0); //holds the current CNT value

      if ((wait_cnt <= max_load) && (wait_cnt <= current_ticks)) //wait time is less than the current time
        {

          LETIMER_CompareSet (LETIMER0, 1, current_ticks - wait_cnt); //load wait_cnt in the timer COMP1
          LETIMER_IntClear (LETIMER0, 0xFFFFFFFF); //clear the interrupts
          LETIMER_IntEnable (LETIMER0, LETIMER_IEN_COMP1); // generate Comp1 interrupt
        }
      else
        {
          wrap_cnt = (max_load - wait_cnt) + current_ticks; //wrap_around
          LETIMER_CompareSet (LETIMER0, 1, wrap_cnt); //load wait_cnt in the timer COMP1
          LETIMER_IntClear (LETIMER0, 0xFFFFFFFF);
          LETIMER_IntEnable (LETIMER0, LETIMER_IEN_COMP1); // generate an interrupt
          //LETIMER0->IEN |= LETIMER_IEN_COMP1; //new
        }
      LETIMER_Enable (LETIMER0, true);
    } //in the range

  else //wait time is out of range
    {
      LOG_ERROR("\n\r OUT OF RANGE,Invalid wait time: %u\n", us_wait); //log error
    } // wait time is more than the max load cnt
}

//timer wait by polling in micro-seconds
void timerWaitUs_polled(uint32_t us_wait)
{
  uint32_t current_ticks, max_load, wait_cnt, wrap_cnt; // all time values with respect to ticks
  if ((us_wait <= MAX_WAIT) && (us_wait >= MIN_WAIT)) // in the range
    {
      wait_cnt = (us_wait * ULFRCO_FREQ) / US_PER_S; // truncation
      max_load = COMP0_LOAD; // maximum value of the ticks for ulfrco
      current_ticks = LETIMER_CounterGet (LETIMER0); //holds the current CNT value

      if ((wait_cnt <= max_load) && (wait_cnt <= current_ticks)) //wait time is less than the current time
        {
          while (LETIMER_CounterGet (LETIMER0) > (current_ticks - wait_cnt))
            {} // wait
        }
      else
        {
          while (!(LETIMER_IntGet (LETIMER0) & LETIMER_IF_UF))
            {}  // Busy wait for underflow
          wrap_cnt = wait_cnt - current_ticks;
          while (LETIMER_CounterGet (LETIMER0) != wrap_cnt) //wrapping up
            {} //the additional time to wait
        }
    } //in the range

  else //wait time is out of range
    {
      LOG_ERROR("\n\r OUT OF RANGE,Invalid wait time: %u\n", us_wait); //log error
    } // wait time is more than the max load cnt
}


//unity test function of the timer delay via non-blocking(interrupts)
void unit_test_timerWaitIrq() {

  GPIO_PinOutToggle (gpioPortF, 5);// small delay
  timerWaitUs_irq(10000);  // 10ms
  GPIO_PinOutToggle (gpioPortF, 5);

  GPIO_PinOutToggle (gpioPortF, 5);// medium delay
  timerWaitUs_irq(100000);  // 100ms
  GPIO_PinOutToggle (gpioPortF, 5);

  GPIO_PinOutToggle (gpioPortF, 5);// large delay
  timerWaitUs_irq(1000000);  // 1s
  GPIO_PinOutToggle (gpioPortF, 5);

  GPIO_PinOutToggle (gpioPortF, 5);// out-of-range value
  timerWaitUs_irq(4000000);       //4s
  GPIO_PinOutToggle (gpioPortF, 5);

  GPIO_PinOutToggle (gpioPortF, 5);// negative value
  timerWaitUs_irq(-10000);
  GPIO_PinOutToggle (gpioPortF, 5);

}
