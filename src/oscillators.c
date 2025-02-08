/*oscillators.c
   This file includes the initialization and configuration of the oscillators
       Author:@Aysvarya Gopinath   Aysvarya.Gopinath@Colorado.edu
 *
 */

//i) low power mode (EM3) --ULFRCO (1 KHz)
//ii)higher power modes (EM0/EM1/EM2)-- LFXO (32.768 KHz)
#include "em_cmu.h"
#include <stdbool.h>
#include <stdint.h>

void oscillator_config(){
  CMU_OscillatorEnable(cmuOsc_ULFRCO,true,true); //enable clock
  CMU_ClockSelectSet(cmuClock_LFA,cmuSelect_ULFRCO); //set source //1000hz
  CMU_ClockDivSet (cmuClock_LETIMER0, cmuClkDiv_1); //prescaler
  CMU_ClockEnable(cmuClock_LETIMER0,true); //enable clock
}


