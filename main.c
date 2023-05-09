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

#include "pic32mx.h"

#include <xc.h>
#include <sys/attribs.h>
#include <string.h>

#include "pic32mx.h"
#include "gpio.h"
#include "uart.h"
#include "mrubyc.h"

#include "model_dependent.c"  // include system (CPU) related functions.


#define MEMORY_SIZE (1024*40)
uint8_t memory_pool[MEMORY_SIZE];


void mrbc_init_class_adc(void);
void mrbc_init_class_pwm(void);
void mrbc_init_class_i2c(void);
void mrbc_init_class_spi(void);
void add_code(void);

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
  switch( val->tt ) {
  case MRBC_TT_INTEGER: {
    int ch = mrbc_integer(*val);
    if( ch <= 4 ) {		// Rboard J9,J10,J11 mapping.
      pin_handle->port = 1;
      pin_handle->num = ch;
    } else {
      pin_handle->port = 2;
      pin_handle->num = ch-5;
    }
  } break;

  case MRBC_TT_STRING: {
    const char *s = mrbc_string_cstr(val);
    if( 'A' <= s[0] && s[0] <= 'G' ) {
      pin_handle->port = s[0] - 'A' + 1;
    } else if( 'a' <= s[0] && s[0] <= 'g' ) {
      pin_handle->port = s[0] - 'a' + 1;
    } else {
      return -1;
    }

    pin_handle->num = mrbc_atoi( s+1, 10 );
  } break;

  default:
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


/*!
  main function
*/
int main(void)
{
  /* module init */
  system_init();
  uart_init();

  if( check_timeout() ) {
    /* IDE code */
    add_code();
    memset( memory_pool, 0, sizeof(memory_pool) );
  }
  mrbc_printf("\r\n\x1b(B\x1b)B\x1b[0m\x1b[2JRboard v*.*, mruby/c v3.2 start.\n");

  /* start mruby/c */
  mrbc_init(memory_pool, MEMORY_SIZE);

  mrbc_init_class_gpio();
  mrbc_init_class_uart();
  mrbc_init_class_adc();
  mrbc_init_class_pwm();
  mrbc_init_class_i2c();
  mrbc_init_class_spi();

  tick_timer_init();

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
