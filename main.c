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

#include "pic32mx.h"
#include "gpio.h"
#include "uart.h"
#include "mrubyc.h"

#include "model_dependent.c"  // include system (CPU) related functions.


// mruby/c heap
#if !defined(MRBC_MEMORY_SIZE)
#define MRBC_MEMORY_SIZE (1024*40)
#endif
uint8_t memory_pool[MRBC_MEMORY_SIZE];


// function prototypes.
void tick_timer_init( void );
void mrbc_init_class_adc(void);
void mrbc_init_class_pwm(void);
void mrbc_init_class_i2c(void);
void mrbc_init_class_spi(void);
int receive_bytecode( void *buffer, int buffer_size );
void * pickup_task( void *task );



//================================================================
/*
  HAL functions.
*/
int hal_write(int fd, const void *buf, int nbytes) {
  return uart_write( UART_HANDLE_CONSOLE, buf, nbytes );
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
  uart_write( UART_HANDLE_CONSOLE, &c, 1 );
}


//================================================================
/*
  on board devices
*/
/*! control the onboard LEDs

  leds_write( n )  # n = 0 to 0b1111 (bit mapped)
*/
static void c_leds_write(mrbc_vm *vm, mrbc_value v[], int argc)
{
  int led = GET_INT_ARG(1);

  onboard_led( 1, led & 0x01 );
  onboard_led( 2, led & 0x02 );
  onboard_led( 3, led & 0x04 );
  onboard_led( 4, led & 0x08 );
}

/*! read the onboard switch

  x = sw()
*/
static void c_sw(mrbc_vm *vm, mrbc_value v[], int argc)
{
  SET_INT_RETURN( onboard_sw(1) );
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
    if( uart_can_read_line( UART_HANDLE_CONSOLE ) ) return 1;
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
    receive_bytecode( memory_pool, MRBC_MEMORY_SIZE );
    memset( memory_pool, 0, MRBC_MEMORY_SIZE );
  }
  mrbc_printf("\r\n\x1b(B\x1b)B\x1b[0m\x1b[2JRboard v2.1.0, mruby/c v3.3.1 start.\n");

  /* start mruby/c */
  mrbc_init(memory_pool, MRBC_MEMORY_SIZE);

  mrbc_init_class_gpio();
  mrbc_init_class_uart();
  mrbc_init_class_adc();
  mrbc_init_class_pwm();
  mrbc_init_class_i2c();
  mrbc_init_class_spi();

  mrbc_define_method(0, 0, "leds_write", c_leds_write);
  mrbc_define_method(0, 0, "sw", c_sw);

  tick_timer_init();

#if 1
  void *task = 0;
  while( 1 ) {
    task = pickup_task( task );
    if( task == 0 ) break;

    mrbc_create_task( task, 0 );
  }

#else
  /* Or run the prepared bytecode.

     How to create "prepared_bytecode.c"
       mrbc --remove-lv -B bytecode -o prepared_bytecode.c *.rb
  */
  #include "prepared_bytecode.c"
  mrbc_printf("prepared bytecode executing.\n");
  mrbc_create_task(bytecode, 0);
#endif

  /* and run! */
  mrbc_run();

  return 1;
}
