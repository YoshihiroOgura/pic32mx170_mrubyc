/* ************************************************************************** */
/** UART

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    uart.c

  @Summary
    UART processing

  @Description
    mruby/c function army
 */
/* ************************************************************************** */

#include "uart.h"
#include <sys/attribs.h>

/* ================================ C codes ================================ */

#if !defined(PBCLK)
# define PBCLK 10000000UL
#endif

// global variables.
UART_HANDLE uart1_handle, uart2_handle;

// function prototypes.
static void uart_push_rxfifo( UART_HANDLE *uh, uint8_t ch );


//================================================================
/*! UART1 interrupt handler.
*/
void __ISR(_UART_1_VECTOR, IPL4AUTO) uart1_isr( void )
{
  if( IFS1bits.U1RXIF ) {
    do {
      uart_push_rxfifo( &uart1_handle, U1RXREG );
    } while( U1STAbits.URXDA );
    IFS1CLR = (1 << _IFS1_U1RXIF_POSITION);
  }

  if( U1STAbits.FERR ) {
    U1STACLR = (1 << _U1STA_FERR_POSITION);
  }
  if( U1STAbits.OERR ) {
    U1STACLR = (1 << _U1STA_OERR_POSITION);
    uart1_handle.rx_overflow = 1;
  }
}

/*! UART2 interrupt handler.
*/
void __ISR(_UART_2_VECTOR, IPL4AUTO) uart2_isr( void )
{
  if( IFS1bits.U2RXIF ) {
    do {
      uart_push_rxfifo( &uart2_handle, U2RXREG );
    } while( U2STAbits.URXDA );
    IFS1CLR = (1 << _IFS1_U2RXIF_POSITION);
  }

  if( U2STAbits.FERR ) {
    U2STACLR = (1 << _U2STA_FERR_POSITION);
  }
  if( U2STAbits.OERR ) {
    U2STACLR = (1 << _U2STA_OERR_POSITION);
    uart2_handle.rx_overflow = 1;
  }
}

static void uart_push_rxfifo( UART_HANDLE *uh, uint8_t ch )
{
  uh->rxfifo[uh->rx_wr++] = ch;

  // check rollover write index.
  if( uh->rx_wr < sizeof(uh->rxfifo)) {
    if( uh->rx_wr == uh->rx_rd ) {
      --uh->rx_wr;   // buffer full
      uh->rx_overflow = 1;
    }
  }
  else {
    if( uh->rx_rd == 0 ) {
      --uh->rx_wr;   // buffer full
      uh->rx_overflow = 1;
    }
    else {
      uh->rx_wr = 0; // roll over.
    }
  }
}


//================================================================
/*! set baud-rate

  @param  baudrate	baudrate.
*/
static void uart1_set_baudrate( int baudrate )
{
  /* データシート掲載計算式
       UxBRG = PBCLK / (4 * baudrate) - 1
     戦略
     　誤差を小さくしたい。
     　4bitシフトして計算した後3bit目を足すことで四捨五入の代わりにする。
  */
  uint32_t brg_x16 = ((uint32_t)PBCLK << 2) / baudrate;
  U1BRG = (brg_x16 >> 4) + ((brg_x16 & 0xf) >> 3) - 1;
}

static void uart2_set_baudrate( int baudrate )
{
  uint32_t brg_x16 = ((uint32_t)PBCLK << 2) / baudrate;
  U2BRG = (brg_x16 >> 4) + ((brg_x16 & 0xf) >> 3) - 1;
}


//================================================================
/*! clear rx buffer and hardware FIFO
*/
static void uart1_clear_rx_buffer( UART_HANDLE *uh )
{
  IEC1bits.U1RXIE = 0;	// Disable Rx interrupt

  while( U1STAbits.URXDA ) {
    volatile uint8_t ch = U1RXREG; (void)ch;
  }
  uh->rx_rd = 0;
  uh->rx_wr = 0;
  uh->rx_overflow = 0;
  IEC1bits.U1RXIE = 1;	// Enable Rx interrupt.
}

static void uart2_clear_rx_buffer( UART_HANDLE *uh )
{
  IEC1bits.U2RXIE = 0;	// Disable Rx interrupt

  while( U2STAbits.URXDA ) {
    volatile uint8_t ch = U2RXREG; (void)ch;
  }
  uh->rx_rd = 0;
  uh->rx_wr = 0;
  uh->rx_overflow = 0;
  IEC1bits.U2RXIE = 1;	// Enable Rx interrupt.
}


//================================================================
/*! Send out binary data.

  @param  buffer        Pointer of buffer.
  @param  size          Size of buffer.
  @return               Size of transmitted.
*/
static int uart1_write( const void *buffer, int size )
{
  const uint8_t *p = (const uint8_t *)buffer;
  int n = size;

  while( n > 0 ) {
    while( U1STAbits.UTXBF ) {	// TX-FIFOに空きができるまで待つ。
      Nop(); Nop(); Nop(); Nop();
    }
    U1TXREG = *p++;
    n--;
  }

  return size;
}

static int uart2_write( const void *buffer, int size )
{
  const uint8_t *p = (const uint8_t *)buffer;
  int n = size;

  while( n > 0 ) {
    while( U2STAbits.UTXBF ) {	// TX-FIFOに空きができるまで待つ。
      Nop(); Nop(); Nop(); Nop();
    }
    U2TXREG = *p++;
    n--;
  }

  return size;
}


//================================================================
/*! initializer
*/
void uart_init(void)
{
  /* UART1
      TxD: RPB4(11pin)
      RxD: RPA4(12pin)
  */
  uart1_handle.delimiter = '\n';
  uart1_handle.set_baudrate = uart1_set_baudrate;
  uart1_handle.clear_rx_buffer = uart1_clear_rx_buffer;
  uart1_handle.write = uart1_write;

  TRISBbits.TRISB4 = 0;		// RB4->output
  RPB4Rbits.RPB4R = 0x0001;	// RB4->UART1:U1TX
  TRISAbits.TRISA4 = 1;		// RA4->input
  U1RXRbits.U1RXR = 0x0002;	// RA4->UART1:U1RX

  // UART1 parameter.
  U1MODE = 0x0008;
  U1STA = 0x0;
  U1TXREG = 0x0;
  uart1_set_baudrate( 19200 );

  // interrupt.
  IPC8bits.U1IP = 4;		// interrupt level
  IPC8bits.U1IS = 3;
  IEC1bits.U1RXIE = 1;		// Rx interrupt

  // Enabling UART1
  U1STAbits.UTXEN = 1;		// TX_enable
  U1STAbits.URXEN = 1;		// RX_enable
  U1MODEbits.ON = 1;

  /* UART2
      TxD: RPB9(18pin)
      RxD: RPB8(17pin)
  */
  uart2_handle.delimiter = '\n';
  uart2_handle.set_baudrate = uart2_set_baudrate;
  uart2_handle.clear_rx_buffer = uart2_clear_rx_buffer;
  uart2_handle.write = uart2_write;

  TRISBbits.TRISB9 = 0;		// RB9->output
  RPB9Rbits.RPB9R = 0x0002;	// RB9->UART2:U2TX
  TRISBbits.TRISB8 = 1;		// RB8->input
  U2RXRbits.U2RXR = 0x0004;	// RB8->UART2:U2RX

  // UART2 parameter.
  U2MODE = 0x0008;
  U2STA = 0x0;
  U2TXREG = 0x0;
  uart2_set_baudrate( 19200 );

  // interrupt.
  IPC9bits.U2IP = 4;		// interrupt level
  IPC9bits.U2IS = 3;
  IEC1bits.U2RXIE = 1;		// Rx interrupt

  // Enabling UART2
  U2STAbits.UTXEN = 1;		// TX_enable
  U2STAbits.URXEN = 1;		// RX_enable
  U2MODEbits.ON = 1;
}


//================================================================
/*! Receive binary data.

  @memberof UART_HANDLE
  @param  uh		Pointer of UART_HANDLE.
  @param  buffer	Pointer of buffer.
  @param  size		Size of buffer.
  @return int		Num of received bytes.

  @note			If no data received, it blocks execution.
*/
int uart_read( UART_HANDLE *uh, void *buffer, size_t size )
{
  if( size == 0 ) return 0;

  // wait for data.
  while( !uart_is_readable(uh) ) {
    Nop(); Nop(); Nop(); Nop();
  }

  // copy fifo to buffer
  uint8_t *buf = buffer;
  size_t   cnt = size;
  uint16_t rx_rd;

  do {
    rx_rd = uh->rx_rd;
    *buf++ = uh->rxfifo[rx_rd++];
    if( rx_rd >= sizeof(uh->rxfifo) ) rx_rd = 0;
    uh->rx_rd = rx_rd;
  } while( --cnt != 0 && rx_rd != uh->rx_wr );

  return size - cnt;
}


//================================================================
/*! check data length can be read.

  @memberof UART_HANDLE
  @param  uh		Pointer of UART_HANDLE.
  @return int		result (bytes)
*/
int uart_bytes_available( UART_HANDLE *uh )
{
  uint16_t rx_wr = uh->rx_wr;

  if( uh->rx_rd <= rx_wr ) {
    return rx_wr - uh->rx_rd;
  }
  else {
    return sizeof(uh->rxfifo) - uh->rx_rd + rx_wr;
  }
}


//================================================================
/*! check data can be read a line.

  @memberof UART_HANDLE
  @param  uh		Pointer of UART_HANDLE.
  @return int		string length.
  @note
   If RX-FIFO buffer is full, return -1.
*/
int uart_can_read_line( UART_HANDLE *uh )
{
  uint16_t idx   = uh->rx_rd;
  uint16_t rx_wr = uh->rx_wr;

  if( uh->rx_overflow ) return -1;

  while( idx != rx_wr ) {
    if( uh->rxfifo[idx++] == uh->delimiter ) {
      if( uh->rx_rd < idx ) {
	return idx - uh->rx_rd;
      } else {
	return sizeof(uh->rxfifo) - uh->rx_rd + idx;
      }
    }
    if( idx >= sizeof(uh->rxfifo)) idx = 0;
  }

  return 0;
}




void u_puts(char *mo, int size){
    if(size == 0){
        size = strlen(mo);
    }

    uart1_write( mo, size );
}

int u_read(char *addr){
  int n;

  do {
    n = uart_can_read_line( &uart1_handle );
  } while( n == 0 );

  return uart_read( &uart1_handle, addr, 50 );
}

/* ============================= mruby/c codes ============================= */


//================================================================
/*! UART constructor

  $uart = UART.new( baud )	// TODO incomplete compatibility.
*/
static void c_uart_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
#if 0
  if( argc == 1 && mrbc_type(v[1]) == MRBC_TT_FIXNUM ) {
    uart2_set_baudrate( mrbc_fixnum(v[1]) );
  }
#else
  uart2_set_baudrate( GET_INT_ARG(1) );
#endif

  *v = mrbc_instance_new(vm, v->cls, sizeof(UART_HANDLE *));
  *((UART_HANDLE **)v->instance->data) = &uart2_handle;
  return;
}


//================================================================
/*! read

  s = $uart.read(n)

  @param  n		Number of bytes receive.
  @return String	Received data.
  @return Nil		Not enough receive length.
*/
static void c_uart_read(mrbc_vm *vm, mrbc_value v[], int argc)
{
  mrbc_value ret;
  UART_HANDLE *handle = *(UART_HANDLE **)v->instance->data;
  int need_length = GET_INT_ARG(1);

  if( uart_is_rx_overflow( handle ) ) {
    console_print("UART Rx buffer overflow. resetting.\n");
    uart_clear_rx_buffer( handle );
    goto RETURN_NIL;
  }

  if( uart_bytes_available(handle) < need_length ) {
    goto RETURN_NIL;
  }

  char *buf = mrbc_alloc( vm, need_length + 1 );
  if( !buf ) {
    goto RETURN_NIL;
  }

  int readed_length = uart_read( handle, buf, need_length );
  if( readed_length < 0 ) {
    goto RETURN_NIL;
  }

  ret = mrbc_string_new_alloc( vm, buf, readed_length );
  goto DONE;

 RETURN_NIL:
  ret = mrbc_nil_value();

 DONE:
  SET_RETURN(ret);
}


//================================================================
/*! read_nonblock

  s = $uart.read_nonblock(maxlen)

  @param  maxlen	Maximum receive length.
  @return String	Received data.
  @return Nil		No received data
*/
static void c_uart_read_nonblock(mrbc_vm *vm, mrbc_value v[], int argc)
{
  mrbc_value ret;
  UART_HANDLE *handle = *(UART_HANDLE **)v->instance->data;
  int max_length = GET_INT_ARG(1);

  int len = uart_bytes_available(handle);
  if( len == 0 ) {
    ret = mrbc_nil_value();
    goto DONE;
  }

  if( len > max_length ) len = max_length;

  char *buf = mrbc_alloc( vm, len + 1 );
  uart_read( handle, buf, len );

  ret = mrbc_string_new_alloc( vm, buf, len );

 DONE:
  SET_RETURN(ret);
}


//================================================================
/*! write

  $uart.write(s)

  @param  s	  Write data.
*/
static void c_uart_write(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *handle = *(UART_HANDLE **)v->instance->data;

  switch( v[1].tt ) {
  case MRBC_TT_STRING: {
    int n = uart_write( handle,
			mrbc_string_cstr(&v[1]), mrbc_string_size(&v[1]) );
    SET_INT_RETURN(n);
  } break;

  default:
    SET_NIL_RETURN();
    break;
  }
}


//================================================================
/*! gets

  s = $uart.gets()
  @return String	Received string.
  @return Nil		Not enough receive length.
*/
static void c_uart_gets(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *handle = *(UART_HANDLE **)v->instance->data;

  int len = uart_can_read_line(handle);
  if( len == 0 ) goto NIL_RETURN;
  if( len <  0 ) {
    console_print("UART Rx buffer overflow. resetting.\n");
    uart_clear_rx_buffer( handle );
    goto NIL_RETURN;
  }

  char *buf = mrbc_alloc( vm, len+1 );
  if( !buf ) goto NIL_RETURN;

  uart_read( handle, buf, len );

  mrbc_value ret = mrbc_string_new_alloc( vm, buf, len );
  SET_RETURN(ret);
  return;

 NIL_RETURN:
  SET_NIL_RETURN();
}


//================================================================
/*! clear_tx_buffer

  $uart.clear_tx_buffer()
*/
static void c_uart_clear_tx_buffer(mrbc_vm *vm, mrbc_value v[], int argc)
{
  // nothing todo, yet.
}


//================================================================
/*! clear_rx_buffer

  $uart.clear_rx_buffer()
*/
static void c_uart_clear_rx_buffer(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *handle = *(UART_HANDLE **)v->instance->data;
  uart_clear_rx_buffer( handle );
}


//================================================================
/*! initialize
*/
void mrbc_init_class_uart(struct VM *vm)
{
  // define class and methods.
  mrbc_class *uart;
  uart = mrbc_define_class(0, "UART",		0 );

  mrbc_define_method(0, uart, "new",		c_uart_new);
  mrbc_define_method(0, uart, "read",		c_uart_read);
  mrbc_define_method(0, uart, "read_nonblock",	c_uart_read_nonblock);
  mrbc_define_method(0, uart, "write",		c_uart_write);
  mrbc_define_method(0, uart, "gets",		c_uart_gets);
  mrbc_define_method(0, uart, "puts",		c_uart_write);
  mrbc_define_method(0, uart, "clear_tx_buffer", c_uart_clear_tx_buffer);
  mrbc_define_method(0, uart, "clear_rx_buffer", c_uart_clear_rx_buffer);
}
