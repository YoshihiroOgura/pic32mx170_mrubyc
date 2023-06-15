/*
  PIC32MX Related functions.
*/

#include <sys/kmem.h>
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



//================================================================
/*! NVM unlock and execute nvm operation.

  @param nvmop	NVM Operation bits. (see: DS60001121G, Sect 5.2.1)
  @return	not zero if errors.
*/
unsigned int NVMUnlock(unsigned int nvmop)
{
  unsigned int status;

  // Suspend or Disable all Interrupts
  asm volatile ("di %0" : "=r" (status));

  // clearing error bits before performing an NVM operation
  NVMCONCLR = 0x0f;

  // Enable Flash Write/Erase Operations and Select
  // Flash operation to perform
  NVMCON = (0x4000 | nvmop);

  // Write Keys
  NVMKEY = 0xAA996655;
  NVMKEY = 0x556699AA;

  // Start the operation using the Set Register
  NVMCONSET = 0x8000;

  // Wait for operation to complete
  while (NVMCON & 0x8000);

  // Restore Interrupts
  if( status & 0x00000001 ) {
    asm volatile ("ei");
  } else {
    asm volatile ("di");
  }

  // Disable NVM write enable
  NVMCONCLR = 0x4000;

  // Return WRERR and LVDERR Error Status Bits
  return (NVMCON & 0x3000);
}


//================================================================
/*! Erase one *PAGE* of FLASH ROM

  @param  address	flash rom address
  @return		not zero if errors.
*/
unsigned int flash_erase_page(void *address)
{
  // Set NVMADDR to the Start Address of page to erase
  NVMADDR = KVA_TO_PA(address);

  // Unlock and Erase Page
  return NVMUnlock( 0x4 );	// 0x4 = Page Erase Operation
}


//================================================================
/*! Write one *ROW* of FLASH ROM

  @param  address	flash rom address
  @param  data		write data address
  @return		not zero if errors.
*/
unsigned int flash_write_row(void *address, void *data)
{
  // Set NVMADDR to Start Address of row to program
  NVMADDR = KVA_TO_PA(address);

  // Set NVMSRCADDR to the SRAM data buffer Address
  NVMSRCADDR = KVA_TO_PA(data);

  // Unlock and Write Row
  return NVMUnlock( 0x3 );	// 0x3 = Row Program Operation
}
