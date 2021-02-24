/*
 * File:   delay.c
 * Author: 164
 *
 * Created on July 25, 2019, 11:13 AM
 */

#include <xc.h>
#include "delay.h"

#if !defined(_XTAL_FREQ)
#define _XTAL_FREQ 20000000
#endif


static void delay_ticks( uint32_t ticks )
{
  uint32_t t_start = _CP0_GET_COUNT();

  while( (_CP0_GET_COUNT() - t_start) < ticks )
    ;
}

// emulate PIC16/24 delay function.
void __delay_us( uint32_t us )
{
  delay_ticks( us * (_XTAL_FREQ/2/1000000) );
}

void __delay_ms( uint32_t ms )
{
  delay_ticks( ms * (_XTAL_FREQ/2/1000) );
}
