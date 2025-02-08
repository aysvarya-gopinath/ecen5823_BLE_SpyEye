/*
  scheduler.c
  This file contains the functions of the event scheduler
  Created on: Feb 3, 2025
   Author:@Aysvarya Gopinath   Aysvarya.Gopinath@Colorado.edu
 *
 */
#include "scheduler.h"
#include"em_core_generic.h"


uint32_t  myEvents=0,setEvent=0;

//scheduler routine to set a scheduler event to the read the temperature
void schedulerSetEvent_ReadTemp() {
  CORE_DECLARE_IRQ_STATE;
  CORE_ENTER_CRITICAL();//enter critical section
  myEvents |= READ_TEMPERATURE; // RMW 0xb0011
  CORE_EXIT_CRITICAL(); // exit critical section
}

// scheduler routine to return 1 event to process
uint32_t getNextEvent() {
  CORE_DECLARE_IRQ_STATE;
  if (myEvents & READ_TEMPERATURE)
    {
      setEvent = READ_TEMPERATURE;
      CORE_ENTER_CRITICAL(); //enter critical section
      myEvents &= ~READ_TEMPERATURE; // clear the bit after processing
      CORE_EXIT_CRITICAL(); // exit critical section
    }
  return (setEvent);
}



