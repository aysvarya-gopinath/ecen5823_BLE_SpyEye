/*
  irq.h
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
#ifndef SRC_IRQ_H_
#define SRC_IRQ_H_

void LETIMER0_IRQHandler(void) ;

#endif /* SRC_IRQ_H_ */
