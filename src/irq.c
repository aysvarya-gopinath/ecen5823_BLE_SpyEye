/*
  irq.c
   This file includes configuration of LETIMER for periodic interrupts at different power modes
       Author: @Aysvarya Gopinath   Aysvarya.Gopinath@Colorado.edu
 *
 *
 */
#include "em_letimer.h"
#include "timers.h"
#include "gpio.h"

#include "scheduler.h"
#include"em_core_generic.h"

void LETIMER0_IRQHandler (void)
{
  CORE_DECLARE_IRQ_STATE;

  uint32_t flags = LETIMER_IntGetEnabled (LETIMER0); // Get only enabled interrupts
  LETIMER_IntClear (LETIMER0, flags);  // Clear pending interrupts
  CORE_ENTER_CRITICAL(); // disable NVIC interrupts
       if (flags & LETIMER_IF_UF)// Underflow interrupt
      {
        schedulerSetEvent_ReadTemp(); // set an event if timer underflows
      }
 CORE_EXIT_CRITICAL(); //exiting critical sections
}

