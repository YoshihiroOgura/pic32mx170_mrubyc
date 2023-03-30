/* ************************************************************************** */

/** main

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    main.c

  @Summary
    mruby/c firmware for Rboard.

  @Description
    main routine.
 */
/* ************************************************************************** */

#include <xc.h>
#include <sys/attribs.h>
#include <string.h>

#include "common.h"
#include "uart.h"
#include "mrbc_firm.h"



// TODO refactoring

#include <stdio.h>
#include <stdlib.h>
//#include <math.h>

#include "mrubyc.h"
//#include "adc.h"
#include "digital.h"
//#include "i2c.h"
//#include "spi.h"
#include "uart.h"

// /TODO refactoring



#define MEMORY_SIZE (1024*40)
uint8_t memory_pool[MEMORY_SIZE];



#include "pic32mx.c"	// include system (CPU) related functions.

/*
  system common functions
*/
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
/*
  HAL functions.
*/
int hal_write(int fd, const void *buf, int nbytes) {
  return uart_write( UART_CONSOLE, buf, nbytes );
}

int hal_flush(int fd) {
    return 0;
}

void hal_abort( const char *s )
{
  if( s ) {
    hal_write( 0, s, strlen(s) );
  }
  __delay_ms(5000);
  system_reset();
}

void _mon_putc( char c )
{
  uart_write( UART_CONSOLE, &c, 1 );
}


//================================================================
/*! PIN handle setter

  valが、ピン番号（数字）でもポート番号（e.g."B3"）でも受け付ける。

  @param  pin_handle	dist.
  @param  val		src.
  @retval 0		No error.
*/
int set_pin_handle( PIN_HANDLE *pin_handle, const mrbc_value *val )
{
  if( val->tt == MRBC_TT_INTEGER ) {
    int ch = mrbc_integer(*val);
    if( ch <= 4 ) {		// Rboard J9,J10,J11 mapping.
      pin_handle->port = 1;
      pin_handle->num = ch;
    } else {
      pin_handle->port = 2;
      pin_handle->num = ch-5;
    }

  } else if( val->tt == MRBC_TT_STRING ) {
    const char *s = mrbc_string_cstr(val);
    if( 'A' <= s[0] && s[0] <= 'G' ) {
      pin_handle->port = s[0] - 'A' + 1;
    } else if( 'a' <= s[0] && s[0] <= 'g' ) {
      pin_handle->port = s[0] - 'a' + 1;
    } else {
      return -1;
    }

    pin_handle->num = mrbc_atoi( s+1, 10 );

  } else {
    return -1;
  }

  return -(pin_handle->num < 0 || pin_handle->num > 15);
}


/*
  Timer functions.

  using Timer1
        16bit mode.
	1kHz (1ms/cycle)
  see   DS60001105G
        14.3.4.2 16-BIT SYNCHRONOUS COUNTER INITIALIZATION STEPS
*/
static void tick_timer_init( void )
{
  T1CON = 0;		// count PBCLK 1:1
  TMR1 = 0;
  PR1 = PBCLK / 1000;

  IFS_T1IF_clear();
  IPC_T1IPIS( 1, 0 );	// Interrupt priority.
  IEC_T1IE_set( 1 );	// Enable interrupt

  T1CONbits.ON = 1;
}

// Timer1 interrupt handler.
void __ISR(_TIMER_1_VECTOR, IPL1AUTO) timer1_isr( void )
{
  mrbc_tick();
  static int cnt = 0;

  if( ++cnt == 1000 ) {
    onboard_led(2,1);
  } else if( cnt == 2000 ) {
    onboard_led(2,0);
    cnt = 0;
  }

  IFS_T1IF_clear();
}


/*!
  Choose to enter programming mode or run mode.
*/
static int check_timeout( void )
{
  for( int i = 0; i < 50; i++ ) {
    onboard_led( 1, 1 );
    __delay_ms( 30 );
    onboard_led( 1, 0 );
    __delay_ms( 30 );
    if( uart_can_read_line( UART_CONSOLE ) ) return 1;
  }
  return 0;
}



int main(void)
{
  /* module init */
  system_init();
  uart_init();

  tick_timer_init();	// TODO: mrbc_initのあとが良いのでは？



  //    i2c_init();
  //    adc_init();

  __delay_ms( 1000 );
  printf("\r\n\x1b(B\x1b)B\x1b[0m\x1b[2JRboard v*.*, mruby/c v3.2 start.\n");

  if( check_timeout() ) {
    /* IDE code */
    add_code();
    memset( memory_pool, 0, sizeof(memory_pool) );
  };

  /* start mruby/c */
  mrbc_init(memory_pool, MEMORY_SIZE);
  mrbc_define_method(0, mrbc_class_object, "pinInit", (mrbc_func_t)pin_init);
    //    mrbc_init_class_adc(0);
    //    mrbc_init_class_i2c(0);
    //    mrbc_init_class_uart(0);
    //    mrbc_init_class_digital(0);
    //    mrbc_init_class_pwm(0);
    //    mrbc_init_class_onboard(0);
    //    mrbc_init_class_spi(0);


#if 1
  const uint8_t *fl_addr = (uint8_t*)FLASH_SAVE_ADDR;
  static const char RITE[4] = "RITE";
  while( strncmp( (const char *)fl_addr, RITE, sizeof(RITE)) == 0 ) {
    mrbc_create_task(fl_addr, 0);

    // get a next irep.
    uint32_t size = 0;
    for( int i = 0; i < 4; i++ ) {
      size = (size << 8) | fl_addr[8 + i];
    }
    fl_addr += FLASH_ALIGN_ROW_SIZE( size );
  }

#else
  /* Or run the prepared bytecode.

     How to create "prepared_bytecode.c"
       mrbc --remove-lv -B bytecode -o prepared_bytecode.c *.rb
  */
  #include "prepared_bytecode.c"
  mrbc_create_task(bytecode, 0);
#endif

  /* and run! */
  mrbc_run();

  return 1;
}
