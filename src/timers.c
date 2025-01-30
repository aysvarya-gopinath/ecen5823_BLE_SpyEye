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
// Include logging for this file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

void
initLETIMER0 ()
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
// init the timer
  LETIMER_Init (LETIMER0, &letimerInitData);
#if LOWEST_ENERGY_MODE == 3   //ulfrco
LETIMER_CompareSet(LETIMER0,0,COMP0_LOAD);       // load COMP0 (top)
LETIMER_CompareSet(LETIMER0,1,COMP1_LOAD);
#else  //lfxo
  LETIMER_CompareSet (LETIMER0, 0, COMP0_VAL);  //load COMP0 (top)
  LETIMER_CompareSet (LETIMER0, 1, COMP1_VAL); // load COMP1
#endif

// Clear all IRQ flags in the LETIMER0 IF status register
  LETIMER_IntClear (LETIMER0, 0xFFFFFFFF); // punch them all down

// Set UF and COMP1 in LETIMER0_IEN, so that the timer will generate IRQs to the NVIC.
  temp = LETIMER_IEN_UF | LETIMER_IEN_COMP1;

  LETIMER_IntEnable (LETIMER0, temp); // Make sure you have defined the ISR routine LETIMER0_IRQHandler()

// Enable the timer to starting counting down, set LETIMER0_CMD[START] bit, see LETIMER0_STATUS[RUNNING] bit
  LETIMER_Enable (LETIMER0, true);

// read it a few times to make sure it's running within the range of values we expect
  temp = LETIMER_CounterGet (LETIMER0);
  temp = LETIMER_CounterGet (LETIMER0);
  temp = LETIMER_CounterGet (LETIMER0);
} // initLETIMER0()

