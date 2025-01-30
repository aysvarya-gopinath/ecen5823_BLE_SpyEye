/*
  irq.c
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
 *
 */
#include "em_letimer.h"
#include "timers.h"
#include "gpio.h"
#include"em_core_generic.h"

void LETIMER0_IRQHandler (void)
{
  CORE_DECLARE_IRQ_STATE;

  uint32_t flags = LETIMER_IntGetEnabled (LETIMER0); // Get only enabled interrupts

  LETIMER_IntClear (LETIMER0, flags);  // Clear pending interrupts
  CORE_ENTER_CRITICAL(); // disable NVIC interrupts
  if (flags & LETIMER_IF_COMP1)
    {  // COMP1 interrupt (LED on for 175ms)
      gpioLed0SetOn ();  // Turn LED on
    }

  if (flags & LETIMER_IF_UF)
    {  // Underflow interrupt
      gpioLed0SetOff ();  // Turn LED off
    }
  CORE_EXIT_CRITICAL(); //exiting critical sections
}
