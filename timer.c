/* ************************************************************************** */

/** timer

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    timer.c

  @Summary
    mruby/c firmware for Rboard.

  @Description
    tick timer.
 */
/* ************************************************************************** */

#include <xc.h>
#include <sys/attribs.h>

#include "pic32mx.h"
#include "mrubyc.h"


/*
  Tick timer functions.

  using Timer1
        16bit mode.
	1kHz (1ms/cycle)
  see   DS60001105G
        14.3.4.2 16-BIT SYNCHRONOUS COUNTER INITIALIZATION STEPS
*/
void tick_timer_init( void )
{
  T1CON = 0;		// count PBCLK 1:1
  TMR1 = 0;
  PR1 = PBCLK / 1000;

  IFS0CLR = (1 << _IFS0_T1IF_POSITION);
  IPC1bits.T1IP = 1;	// Interrupt priority.
  IPC1bits.T1IS = 0;
  IEC0bits.T1IE = 1;	// Enable interrupt
  T1CONbits.ON = 1;
}

// Timer1 interrupt handler.
void __ISR(_TIMER_1_VECTOR, IPL1AUTO) timer1_isr( void )
{
  mrbc_tick();
  IFS0CLR = (1 << _IFS0_T1IF_POSITION);
}
