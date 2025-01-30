/*oscillators.c
   This file includes the initialization and configuration of the oscillators
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

//i) low power mode (EM3) --ULFRCO (1 KHz)
//ii)higher power modes (EM0/EM1/EM2)-- LFXO (32.768 KHz)
#include "em_cmu.h"
#include <stdbool.h>


void oscillator_config(){
#if LOWEST_ENERGY_MODE == 3
  CMU_OscillatorEnable(cmuOsc_ULFRCO,true,true); //enable clock
  CMU_ClockSelectSet(cmuClock_LFA,cmuSelect_ULFRCO); //set source //1000hz
  CMU_ClockEnable(cmuClock_LETIMER0,true); //enable clock

#else  //EM0,EM1,EM2
  CMU_OscillatorEnable (cmuOsc_LFXO, true, true); //enable clock
  CMU_ClockSelectSet (cmuClock_LFA, cmuSelect_LFXO); //set source
  CMU_ClockDivSet (cmuClock_LETIMER0, cmuClkDiv_4); //prescaler output is 18,432
  CMU_ClockEnable (cmuClock_LETIMER0, true); //enable clock
#endif
}
//uint32_t freq_ = CMU_ClockFreqGet (cmuClock_LFA);

