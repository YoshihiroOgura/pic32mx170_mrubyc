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
      TxD: RPB4(11pin) = Pin9
      RxD: RPA4(12pin) = none
  */
  uart1_handle.number = 1;
  uart1_handle.delimiter = '\n';
  uart1_handle.txd_pin = 9;
  uart1_handle.clear_rx_buffer = uart1_clear_rx_buffer;
  uart1_handle.write = uart1_write;

  // UART1 parameter.
  U1MODE = 0x0008;
  U1STA = 0x0;
  uart_set_modem_params( &uart1_handle, 19200, -1, -1, uart1_handle.txd_pin, 4 );

  // interrupt.
  IPC8bits.U1IP = 4;		// interrupt level
  IPC8bits.U1IS = 3;

  // Enabling UART1
  uart_enable( &uart1_handle, 1 );

  /* UART2
      TxD: RPB9(18pin) = Pin14
      RxD: RPB8(17pin) = Pin13
  */
  uart2_handle.number = 2;
  uart2_handle.delimiter = '\n';
  uart2_handle.txd_pin = 14;
  uart2_handle.clear_rx_buffer = uart2_clear_rx_buffer;
  uart2_handle.write = uart2_write;

  // UART2 parameter.
  U2MODE = 0x0008;
  U2STA = 0x0;
  uart_set_modem_params( &uart2_handle, 19200, -1, -1, uart2_handle.txd_pin, 13 );

  // interrupt.
  IPC9bits.U2IP = 4;		// interrupt level
  IPC9bits.U2IS = 3;

  // Enabling UART2
  uart_enable( &uart2_handle, 1 );
}


//================================================================
/*! enable or disable uart
*/
void uart_enable( const UART_HANDLE *uh, int en_dis )
{
  switch( uh->number ) {
  case 1:
    if( en_dis ) {
      // enable UART1
      IEC1bits.U1RXIE = 1;	// Rx interrupt
      U1STAbits.UTXEN = 1;	// TX enable
      U1STAbits.URXEN = 1;	// RX enable
      U1MODEbits.ON = 1;
    } else {
      //disable
      while( U1STAbits.TRMT == 0 )
	;
      IEC1bits.U1RXIE = 0;
      U1MODEbits.ON = 0;
      U1STAbits.UTXEN = 0;
      U1STAbits.URXEN = 0;
    }
    break;

  case 2:
    if( en_dis ) {
      // enable UART2
      IEC1bits.U2RXIE = 1;	// Rx interrupt
      U2STAbits.UTXEN = 1;	// TX enable
      U2STAbits.URXEN = 1;	// RX enable
      U2MODEbits.ON = 1;
    } else {
      //disable UART2
      while( U2STAbits.TRMT == 0 )
	;
      IEC1bits.U2RXIE = 0;
      U2MODEbits.ON = 0;
      U2STAbits.UTXEN = 0;
      U2STAbits.URXEN = 0;
    }
    break;
  }
}



//================================================================
/*! set parameter
*/
int uart_set_modem_params( UART_HANDLE *uh, int baud, int parity, int stop_bits, int txd, int rxd )
{
  if( baud >= 0 ) {
    /* データシート掲載計算式
         UxBRG = PBCLK / (4 * baudrate) - 1
       戦略
       　誤差を小さくしたい。
       　4bitシフトして計算した後3bit目を足すことで四捨五入の代わりにする。
    */
    uint32_t brg_x16 = ((uint32_t)PBCLK << 2) / baud;
    uint16_t brg = (brg_x16 >> 4) + ((brg_x16 & 0xf) >> 3) - 1;
    switch( uh->number ) {
    case 1: U1BRG = brg; break;
    case 2: U2BRG = brg; break;
    }
  }

  if( 0 <= parity && parity <= 2 ) {
    static const uint8_t pdsel[] = { 0, 2, 1 };
    switch( uh->number ) {
    case 1: U1MODEbits.PDSEL = pdsel[parity]; break;
    case 2: U2MODEbits.PDSEL = pdsel[parity]; break;
    }
  }

  if( 1 <= stop_bits && stop_bits <= 2 ) {
    switch( uh->number ) {
    case 1: U1MODEbits.STSEL = stop_bits - 1; break;
    case 2: U2MODEbits.STSEL = stop_bits - 1; break;
    }
  }

  /* TxD, RxD の設定可能な値
    -----------------------------------
          Reg  Rboard
	        Pin    Device
    -----------------------------------
    U1TX  RA0    0      LED
          RB3    8      I2C
	  RB4    9      (default)
	  RB15  20      Analog
	  RB7   12      SW
    -----------------------------------
    U1RX  RA2    2
          RB6   11
          RA4    4      (default)
          RB13  18
          RB2    7      I2C
    -----------------------------------
    U2TX  RA3    3
          RB14  19      Analog
          RB0    5      LED
          RB10  15      Digital
          RB9   14      (default)
    -----------------------------------
    U2RX  RA1    1      LED
          RB5   10
          RB1    6      LED
          RB11  16      Digital
          RB8   13      (default)
    -----------------------------------
  */
  static const uint8_t u1tx_pins[] = {0,8,9,20,12};
  static const uint8_t u1rx_pins[] = {2,11,4,18,7};
  static const uint8_t u2tx_pins[] = {3,19,5,15,14};
  static const uint8_t u2rx_pins[] = {1,10,6,16,13};

#define RPnR(pin) *((pin) < 5 ? &RPA0R + (pin) : &RPB0R + (pin) - 5)

  if( 0 <= txd && txd <= 20 ) {
    int i;
    switch( uh->number ) {
    case 1:
      for( i = 0; i < sizeof(u1tx_pins); i++ ) {
	if( u1tx_pins[i] == txd ) break;
      }
      if( i < sizeof(u1tx_pins) ) {
	RPnR(uh->txd_pin) = 0;
	RPnR(txd) = 0x1;
	uh->txd_pin = txd;
      }
      break;

    case 2:
      for( i = 0; i < sizeof(u2tx_pins); i++ ) {
	if( u2tx_pins[i] == txd ) break;
      }
      if( i < sizeof(u2tx_pins) ) {
	RPnR(uh->txd_pin) = 0;
	RPnR(txd) = 0x2;
	uh->txd_pin = txd;
      }
      break;
    }
  }

  if( 0 <= rxd && rxd <= 20 ) {
    int i;
    switch( uh->number ) {
    case 1:
      for( i = 0; i < sizeof(u1rx_pins); i++ ) {
	if( u1rx_pins[i] == rxd ) {
	  U1RXRbits.U1RXR = i;
	  break;
	}
      }
      break;

    case 2:
      for( i = 0; i < sizeof(u2rx_pins); i++ ) {
	if( u2rx_pins[i] == rxd ) {
	  U2RXRbits.U2RXR = i;
	  break;
	}
      }
      break;
    }
  }

  return 0;
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
int uart_bytes_available( const UART_HANDLE *uh )
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
int uart_can_read_line( const UART_HANDLE *uh )
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



/* ============================= mruby/c codes ============================= */

//================================================================
/*! UART constructor

  $uart = UART.new( ch, baud )	# ch = 1 or 2
*/
static void c_uart_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  int ch = -1;
  int baud = -1;

  // get parameter.
  if( argc >= 1 ) {
    if( mrbc_type(v[1]) != MRBC_TT_FIXNUM ) goto ERROR_PARAM;
    ch = mrbc_fixnum(v[1]);
  }
  if( argc >= 2 ) {
    if( mrbc_type(v[2]) != MRBC_TT_FIXNUM ) goto ERROR_PARAM;
    baud = mrbc_fixnum(v[2]);
  }

  // in case of UART.new()
  if( ch < 0 ) {
    ch = 2;
  }

  // in case of UART.new(9600), for old version compatibility.
  if( ch >= 300 && baud < 0 ) {
    baud = ch;
    ch = 2;
  }

  // make instance
  mrbc_value val = mrbc_instance_new(vm, v->cls, sizeof(UART_HANDLE *));
  switch( ch ) {
  case 1: *((UART_HANDLE**)val.instance->data) = &uart1_handle; break;
  case 2: *((UART_HANDLE**)val.instance->data) = &uart2_handle; break;
  default: goto ERROR_PARAM;
  }

  // set baudrate.
  if( baud > 0 ) {
    UART_HANDLE *handle = *(UART_HANDLE **)val.instance->data;
    uart_enable( handle, 0 );
    uart_set_modem_params( handle, baud, -1, -1, -1, -1 );
    uart_enable( handle, 1 );
  }

  SET_RETURN( val );
  return;

 ERROR_PARAM:
  console_print("UART parameter error.\n");
}

//================================================================
/*! set_modem_params

  s = $uart.set_modem_params( params... )

  @param  n		Number of bytes receive.
  @return Bool		result.

  (params)
  :baud=>9600	set baud rate.
  :stop_bits=>1	set stop_bits bit, 1 or 2.
  :parity=>0	set parity bit, 0=unused, 1=odd, 2=even.
  :txd=>10	set TxD pin.
  :rxd=>11	set RxD pin.
*/
static void c_uart_set_modem_params(mrbc_vm *vm, mrbc_value v[], int argc)
{
  int baud = -1;
  int parity = -1;
  int stop_bits = -1;
  int txd = -1;
  int rxd = -1;

  if( argc != 1 ) goto ERROR_PARAM;
  if( v[1].tt != MRBC_TT_HASH ) goto ERROR_PARAM;

  // get parameter
  mrbc_hash_iterator ite = mrbc_hash_iterator_new( &v[1] );
  while( mrbc_hash_i_has_next(&ite) ) {
    mrbc_value *kv = mrbc_hash_i_next(&ite);
    if( mrbc_type(kv[0]) != MRBC_TT_STRING ) goto ERROR_PARAM;
    if( mrbc_type(kv[1]) != MRBC_TT_FIXNUM ) goto ERROR_PARAM;

    if( strcmp("baud", RSTRING_PTR(kv[0])) == 0 ) {
      baud = kv[1].i;

    } else if( strcmp("parity", RSTRING_PTR(kv[0])) == 0 ) {
      parity = kv[1].i;

    } else if( strcmp("stop_bits", RSTRING_PTR(kv[0])) == 0 ) {
      stop_bits = kv[1].i;

    } else if( strcmp("txd", RSTRING_PTR(kv[0])) == 0 ) {
      txd = kv[1].i;

    } else if( strcmp("rxd", RSTRING_PTR(kv[0])) == 0 ) {
      rxd = kv[1].i;

    } else {
      goto ERROR_PARAM;
    }
  }

  // set to UART
  UART_HANDLE *handle = *(UART_HANDLE **)v->instance->data;
  uart_enable( handle, 0 );
  if( uart_set_modem_params( handle, baud, parity, stop_bits, txd, rxd ) == 0 ) {
    uart_enable( handle, 1 );
    SET_TRUE_RETURN();
    return;
  }

 ERROR_PARAM:
  console_print("UART parameter error.\n");
  SET_FALSE_RETURN();
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
  mrbc_define_method(0, uart, "set_modem_params", c_uart_set_modem_params);
  mrbc_define_method(0, uart, "read",		c_uart_read);
  mrbc_define_method(0, uart, "read_nonblock",	c_uart_read_nonblock);
  mrbc_define_method(0, uart, "write",		c_uart_write);
  mrbc_define_method(0, uart, "gets",		c_uart_gets);
  mrbc_define_method(0, uart, "puts",		c_uart_write);
  mrbc_define_method(0, uart, "clear_tx_buffer", c_uart_clear_tx_buffer);
  mrbc_define_method(0, uart, "clear_rx_buffer", c_uart_clear_rx_buffer);
}
