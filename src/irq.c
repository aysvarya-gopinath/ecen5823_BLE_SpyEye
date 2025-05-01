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
#include "irq.h"
#include "scheduler.h"
#include"em_core_generic.h"
#include "em_i2c.h"

#include "em_gpio.h"
// Include logging for this file
#define INCLUDE_LOG_DEBUG 0
#include "src/log.h"

 uint32_t roll_over=0; // counter to hold the underflow counts

 /*Letimer interrupt handler
  * No parameters and return types
  */
void LETIMER0_IRQHandler (void)
{
  CORE_DECLARE_IRQ_STATE;

  uint32_t flags = LETIMER_IntGetEnabled (LETIMER0); // Get only enabled interrupts
  LETIMER_IntClear (LETIMER0, flags);  // Clear pending interrupts
  CORE_ENTER_CRITICAL(); // disable NVIC interrupts
  if (flags & LETIMER_IF_UF) // Underflow interrupt
    {
      schedulerSetEvent_underflow (); // set an event if timer underflows
      roll_over++; // increment the counter for every underflow event
    }
  if (flags & LETIMER_IF_COMP1) //comp1 interrupt due to timer waits
    {
      schedulerSetEvent_WaitIrq (); //set an event if the comp1 underflows
    }
  CORE_EXIT_CRITICAL(); //exiting critical sections
}


/*Counts the time elapsed since the sensor power-up
 *Returns time in  milliseconds
 */
uint32_t letimerMilliseconds(void)
{
  uint32_t elapsed_time = COMP0_LOAD - LETIMER_CounterGet (LETIMER0); //count value passed since last UF for resolution
  uint32_t stamp_time = (roll_over * SEC_TO_MS) + elapsed_time; //time equivalent for the number of roll overs +elapsed time
  return stamp_time;
}

/* The default handler for the push button 0 interrupt
 *  No parameters and return types
 */
void GPIO_EVEN_IRQHandler()
{
  CORE_DECLARE_IRQ_STATE;
  uint32_t flags = GPIO_IntGetEnabled (); // Get only enabled and pending interrupts
  GPIO_IntClear(flags); //clear pending interrupts
  CORE_ENTER_CRITICAL(); // disable NVIC interrupts
  if(flags &(1 << PB0_pin) ) //if PB0 interrupt is set
    {
      if ((GPIO_PinInGet(PB_port, PB0_pin) == 0)) // if button pressed
        schedulerSetEvent_PB0_press();
       else if ((GPIO_PinInGet(PB_port, PB0_pin) == 1))
         schedulerSetEvent_PB0_release();
    }
  if(flags &(1 << interrupt_pin) ) //if external gpio interrupt is set
     {
      if (GPIO_PinInGet(PA_port, interrupt_pin) == 0)  // If the interrupt pin is low (0V)
        {
           schedulerSetEvent_Ext_interrupt(); // Handle the interrupt
        }
    }
  CORE_EXIT_CRITICAL(); //exiting critical sections

}

/* The default handler for the push button 1 interrupt
 *  No parameters and return types
 */
void GPIO_ODD_IRQHandler()
{
  CORE_DECLARE_IRQ_STATE;
  uint32_t flags = GPIO_IntGetEnabled (); // Get only enabled and pending interrupts
  GPIO_IntClear(flags); //clear pending interrupts
  CORE_ENTER_CRITICAL(); // disable NVIC interrupts
  if(flags &(1 << PB1_pin) ) //if PB1 interrupt is set
    {
      if ((GPIO_PinInGet(PB_port, PB1_pin) == 0)) // if button pressed
        schedulerSetEvent_PB1_press();
       else if ((GPIO_PinInGet(PB_port, PB1_pin) == 1))
         schedulerSetEvent_PB1_release();
    }
  CORE_EXIT_CRITICAL(); //exiting critical sections
}

