/*
  PIC32MX Related functions.
*/

#include "pic32mx.h"

//================================================================
/*! spin lock delay functions.
*/
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


//================================================================
/*! lock system register
*/
void system_register_lock(void)
{
  SYSKEY = 0x0;
}


//================================================================
/*! unlock system register
*/
void system_register_unlock(void)
{
  unsigned int status;

  // Suspend or Disable all Interrupts
  asm volatile ("di %0" : "=r" (status));

  // starting critical sequence
  SYSKEY = 0x00000000; // write invalid key to force lock
  SYSKEY = 0xAA996655; // write key1 to SYSKEY
  SYSKEY = 0x556699AA; // write key2 to SYSKEY

  // Restore Interrupts
  if (status & 0x00000001) {
    asm volatile ("ei");
  } else {
    asm volatile ("di");
  }
}


//================================================================
/*! software reset	DS60001118H
*/
void system_reset(void)
{
  system_register_unlock();

  RSWRSTSET = 1;

  // read RSWRST register to trigger reset
  uint32_t dummy = RSWRST;
  (void)dummy;

  while(1)
    ;
}


#if 0
//================================================================
/*! general exception handler

    Referred to the MCC auto generated code.
*/
void _general_exception_handler()
{
  static const char *cause[] = {
    "Interrupt", "Undefined", "Undefined", "Undefined",
    "Load/fetch address error", "Store address error",
    "Instruction bus error", "Data bus error", "Syscall",
    "Breakpoint", "Reserved instruction", "Coprocessor unusable",
    "Arithmetic overflow", "Trap", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved"
  };

  /* Mask off the ExcCode Field from the Cause Register
     Refer to the MIPs Software User's manual */

  uint8_t excep_code = (_CP0_GET_CAUSE() & 0x0000007C) >> 2;
  uint8_t excep_addr = _CP0_GET_EPC();
  const uint8_t *cause_str = cause[excep_code];

  uart1_write( cause_str, strlen(cause_str) );
  uart1_write( "\r\n", 2 );

  while(1) {
  }
}
#endif
