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

#include <xc.h>
#include <sys/attribs.h>

#include "pic32mx.h"
#include "gpio.h"
#include "uart.h"
#include "mrubyc.h"


/* ================================ C codes ================================ */
#define UART_NONE	0
#define UART_ODD	1
#define UART_EVEN	2
#define UART_RTSCTS	4
#if defined(MRBC_CONVERT_CRLF)
#define UART_NL		"\r\n"
#else
#define UART_NL		"\n"
#endif


// handle table.
UART_HANDLE uart_handle_[NUM_UART_UNIT];

// function prototypes for static function.
static int uart_assign_pin( const UART_HANDLE *hndl );
static void c_uart_new(mrbc_vm *vm, mrbc_value v[], int argc);
static void c_uart_setmode(mrbc_vm *vm, mrbc_value v[], int argc);
static void c_uart_read(mrbc_vm *vm, mrbc_value v[], int argc);
static void c_uart_write(mrbc_vm *vm, mrbc_value v[], int argc);
static void c_uart_gets(mrbc_vm *vm, mrbc_value v[], int argc);
static void c_uart_puts(mrbc_vm *vm, mrbc_value v[], int argc);
static void c_uart_bytes_available(mrbc_vm *vm, mrbc_value v[], int argc);
static void c_uart_bytes_to_write(mrbc_vm *vm, mrbc_value v[], int argc);
static void c_uart_can_read_line(mrbc_vm *vm, mrbc_value v[], int argc);
static void c_uart_flush(mrbc_vm *vm, mrbc_value v[], int argc);
static void c_uart_clear_tx_buffer(mrbc_vm *vm, mrbc_value v[], int argc);
static void c_uart_clear_rx_buffer(mrbc_vm *vm, mrbc_value v[], int argc);
static void c_uart_send_break(mrbc_vm *vm, mrbc_value v[], int argc);


#if defined(__32MX170F256B__) || defined(__PIC32MX170F256B__)
/*! UART TxD pin setting table
  Pin assign: DS60001168L  TABLE 11-2: OUTPUT PIN SELECTION
*/
static const uint8_t UART_TXD_RPxnR[NUM_UART_UNIT] = {
  0x01,	// U1TX = 0001
  0x02, // U2TX = 0002
};

/*! UART RxD pin settng table
  Pin assign: DS60001168L  TABLE 11-1: INPUT PIN SELECTION
*/
static const uint8_t UART_RXD_PINS[NUM_SPI_UNIT][5] = {
  // U1RX
  // A2    B6    A4    B13   B2
  {0x12, 0x26, 0x14, 0x2d, 0x22},
  // U2RX
  // A1    B   , B1    B11   B8
  {0x11, 0x25, 0x21, 0x2b, 0x28},
};
#else
#include "uart_dependent.h"
#endif


//================================================================
/*! UART1 interrupt handler.
*/
void __ISR(_UART_1_VECTOR, IPL4AUTO) uart1_isr( void )
{
  if( IFS1bits.U1RXIF ) {
    do {
      uart_push_rxfifo( &uart_handle_[0], U1RXREG );
    } while( U1STAbits.URXDA );
    IFS1CLR = (1 << _IFS1_U1RXIF_POSITION);
  }

  if( U1STAbits.FERR ) {
    U1STACLR = (1 << _U1STA_FERR_POSITION);
  }
  if( U1STAbits.OERR ) {
    U1STACLR = (1 << _U1STA_OERR_POSITION);
    uart_handle_[0].rx_overflow = 1;
  }
}

/*! UART2 interrupt handler.
*/
void __ISR(_UART_2_VECTOR, IPL4AUTO) uart2_isr( void )
{
  if( IFS1bits.U2RXIF ) {
    do {
      uart_push_rxfifo( &uart_handle_[1], U2RXREG );
    } while( U2STAbits.URXDA );
    IFS1CLR = (1 << _IFS1_U2RXIF_POSITION);
  }

  if( U2STAbits.FERR ) {
    U2STACLR = (1 << _U2STA_FERR_POSITION);
  }
  if( U2STAbits.OERR ) {
    U2STACLR = (1 << _U2STA_OERR_POSITION);
    uart_handle_[1].rx_overflow = 1;
  }
}

void uart_push_rxfifo( UART_HANDLE *hndl, uint8_t ch )
{
  hndl->rxfifo[hndl->rx_wr++] = ch;

  // check rollover write index.
  if( hndl->rx_wr < sizeof(hndl->rxfifo)) {
    if( hndl->rx_wr == hndl->rx_rd ) {
      --hndl->rx_wr;   // buffer full
      hndl->rx_overflow = 1;
    }
  }
  else {
    if( hndl->rx_rd == 0 ) {
      --hndl->rx_wr;   // buffer full
      hndl->rx_overflow = 1;
    }
    else {
      hndl->rx_wr = 0; // roll over.
    }
  }
}


//================================================================
/*! UART enable or disable interrupt.
*/
#if !defined(UART_INTERRUPT_EN_DIS)
void uart_interrupt_en_dis( const UART_HANDLE *hndl, int en_dis )
{
  switch( hndl->unit_num ) {
  case 1: IEC1bits.U1RXIE = en_dis;
  case 2: IEC1bits.U2RXIE = en_dis;
  }
}
#endif


//================================================================
/*! assign the pin to UART

  @param  hndl	UART HANDLE
  @return	if error, returns minus value.
*/
static int uart_assign_pin( const UART_HANDLE *hndl )
{
  if( hndl->unit_num < 1 || hndl->unit_num > NUM_UART_UNIT ) return -1;

  /* set output (TxD) pin.
     Defaults to high level to keep high level when pin assignment is changed.
  */
  gpio_setmode( &hndl->txd_pin, GPIO_OUT );
  LATxSET(hndl->txd_pin.port) = (1 << hndl->txd_pin.num);  // set high level
  RPxnR(hndl->txd_pin.port, hndl->txd_pin.num) = UART_TXD_RPxnR[hndl->unit_num - 1];

  /* set input (RxD) pin.
  */
  gpio_setmode( &hndl->rxd_pin, GPIO_IN );
  for( int i = 0; i < sizeof(UART_RXD_PINS)/NUM_UART_UNIT; i++ ) {
    if( UART_RXD_PINS[hndl->unit_num-1][i] ==
	(hndl->rxd_pin.port << 4 | hndl->rxd_pin.num) ) {
      UxRXR(hndl->unit_num) = i;
      return 0;
    }
  }

  return -1;
}


//================================================================
/*! initialize unit
*/
#if !defined(UART_INIT)
void uart_init(void)
{
  /*
    UART1
  */
  uart_handle_[0].txd_pin = (PIN_HANDLE){UART1_TXD_PIN};
  uart_handle_[0].rxd_pin = (PIN_HANDLE){UART1_RXD_PIN};
  uart_handle_[0].unit_num = 1;
  uart_handle_[0].delimiter = '\n';

  // UART1 parameter.
  U1MODE = 0x0008;
  U1STA = 0x0;
  uart_setmode( &uart_handle_[0], 19200, 0, 1 );
  uart_assign_pin( &uart_handle_[0] );

  // interrupt level.
  IPC_U1IPIS( 4, 3 );

  // Enabling UART1
  uart_enable( &uart_handle_[0] );

  /*
    UART2
  */
  uart_handle_[1].txd_pin = (PIN_HANDLE){UART2_TXD_PIN};
  uart_handle_[1].rxd_pin = (PIN_HANDLE){UART2_RXD_PIN};
  uart_handle_[1].unit_num = 2;
  uart_handle_[1].delimiter = '\n';

  // UART2 parameter.
  U2MODE = 0x0008;
  U2STA = 0x0;
  uart_setmode( &uart_handle_[1], 9600, 0, 1 );	// I/O API standard baudrate.
  uart_assign_pin( &uart_handle_[1] );

  // interrupt level.
  IPC_U2IPIS( 4, 3 );

  // Enabling UART2
  uart_enable( &uart_handle_[1] );
}
#endif


//================================================================
/*! enable uart

  @memberof UART_HANDLE
*/
void uart_enable( const UART_HANDLE *hndl )
{
  uart_interrupt_enable( hndl );
  UxSTASET(hndl->unit_num) = (_U1STA_UTXEN_MASK | _U1STA_URXEN_MASK);
  UxMODESET(hndl->unit_num) = _U1MODE_ON_MASK;
}


//================================================================
/*! disable uart

  @memberof UART_HANDLE
*/
void uart_disable( const UART_HANDLE *hndl )
{
  while( (UxSTA(hndl->unit_num) & _U1STA_TRMT_MASK) == 0 )
    ;
  uart_interrupt_disable( hndl );
  UxMODECLR(hndl->unit_num) = _U1MODE_ON_MASK;
  UxSTACLR(hndl->unit_num) = (_U1STA_UTXEN_MASK | _U1STA_URXEN_MASK);
}


//================================================================
/*! set mode

  @memberof UART_HANDLE
  @param  baud		baud rate.
  @param  parity	0:none 1:odd 2:even
  @param  stop_bits	1 or 2
  @note いずれも設定変更しないパラメータは、-1 を渡す。
*/
int uart_setmode( const UART_HANDLE *hndl, int baud, int parity, int stop_bits )
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
    UxBRG(hndl->unit_num) = brg;
  }

  if( 0 <= parity && parity <= 2 ) {
    static const uint8_t pdsel[] = { 0, 2, 1 };

    UxMODECLR(hndl->unit_num) = _U1MODE_PDSEL_MASK;
    UxMODESET(hndl->unit_num) = (pdsel[parity] << _U1MODE_PDSEL_POSITION);
  }

  switch(stop_bits) {
  case 1:
    UxMODECLR(hndl->unit_num) = (1 << _U1MODE_STSEL_POSITION);
    break;
  case 2:
    UxMODESET(hndl->unit_num) = (1 << _U1MODE_STSEL_POSITION);
    break;
  }

  return 0;
}


//================================================================
/*! Clear receive buffer.

  @memberof UART_HANDLE
  @param  hndl		Pointer of UART_HANDLE.
*/
void uart_clear_rx_buffer( UART_HANDLE *hndl )
{
  uart_interrupt_disable( hndl );

  while( UxSTA(hndl->unit_num) & _U1STA_URXDA_MASK ) {
    volatile uint8_t ch = UxRXREG(hndl->unit_num); (void)ch;
  }
  hndl->rx_rd = 0;
  hndl->rx_wr = 0;
  hndl->rx_overflow = 0;

  uart_interrupt_enable( hndl );
}


//================================================================
/*! Receive binary data.

  @memberof UART_HANDLE
  @param  hndl		Pointer of UART_HANDLE.
  @param  buffer	Pointer of buffer.
  @param  size		Size of buffer.
  @return int		Num of received bytes.

  @note			If no data received, it blocks execution.
*/
int uart_read( UART_HANDLE *hndl, void *buffer, int size )
{
  if( size == 0 ) return 0;

  // wait for data.
  while( !uart_is_readable(hndl) ) {
    Nop(); Nop(); Nop(); Nop();
  }

  // copy fifo to buffer
  uint8_t *buf = buffer;
  size_t   cnt = size;
  uint16_t rx_rd;

  do {
    rx_rd = hndl->rx_rd;
    *buf++ = hndl->rxfifo[rx_rd++];
    if( rx_rd >= sizeof(hndl->rxfifo) ) rx_rd = 0;
    hndl->rx_rd = rx_rd;
  } while( --cnt != 0 && rx_rd != hndl->rx_wr );

  return size - cnt;
}


//================================================================
/*! Send out binary data.

  @memberof UART_HANDLE
  @param  hndl            Pointer of UART_HANDLE.
  @param  buffer        Pointer of buffer.
  @param  size          Size of buffer.
  @return               Size of transmitted.
*/
int uart_write( UART_HANDLE *hndl, const void *buffer, int size )
{
  const uint8_t *p = (const uint8_t *)buffer;
  int n = size;

  while( n > 0 ) {
    // TX-FIFOに空きができるまで待つ。
    while( UxSTA(hndl->unit_num) & _U1STA_UTXBF_MASK ) {
      Nop(); Nop(); Nop(); Nop();
    }
    UxTXREG(hndl->unit_num) = *p++;
    n--;
  }

  return size;
}


//================================================================
/*! Receive string.

  @memberof UART_HANDLE
  @param  hndl		target UART_HANDLE
  @param  buffer	pointer to buffer.
  @param  size		Size of buffer.
  @return int		Num of received bytes.

  @note			If no data received, it blocks execution.
*/
int uart_gets( UART_HANDLE *hndl, void *buffer, int size )
{
  uint8_t *buf = buffer;
  int len;

  while( 1 ) {
    len = uart_can_read_line(hndl);
    if( len > 0 ) break;

      Nop(); Nop(); Nop(); Nop();
  }

  if( len >= size ) return -1;		// buffer size too small.

  // copy fifo to buffer
  for( int ba = len; ba > 0; ba-- ) {
    *buf++ = hndl->rxfifo[hndl->rx_rd++];
    if( hndl->rx_rd >= sizeof(hndl->rxfifo) ) hndl->rx_rd = 0;
  }
  *buf = '\0';

  return len;
}


//================================================================
/*! check data length can be read.

  @memberof UART_HANDLE
  @param  hndl		Pointer of UART_HANDLE.
  @return int		result (bytes)
*/
int uart_bytes_available( const UART_HANDLE *hndl )
{
  uint16_t rx_wr = hndl->rx_wr;

  if( hndl->rx_rd <= rx_wr ) {
    return rx_wr - hndl->rx_rd;
  }
  else {
    return sizeof(hndl->rxfifo) - hndl->rx_rd + rx_wr;
  }
}


//================================================================
/*! check data can be read a line.

  @memberof UART_HANDLE
  @param  hndl		Pointer of UART_HANDLE.
  @return int		string length.
  @note
   If RX-FIFO buffer is full, return -1.
*/
int uart_can_read_line( const UART_HANDLE *hndl )
{
  uint16_t idx   = hndl->rx_rd;
  uint16_t rx_wr = hndl->rx_wr;

  if( hndl->rx_overflow ) return -1;

  while( idx != rx_wr ) {
    if( hndl->rxfifo[idx++] == hndl->delimiter ) {
      if( hndl->rx_rd < idx ) {
	return idx - hndl->rx_rd;
      } else {
	return sizeof(hndl->rxfifo) - hndl->rx_rd + idx;
      }
    }
    if( idx >= sizeof(hndl->rxfifo)) idx = 0;
  }

  return 0;
}



/* ============================= mruby/c codes ============================= */

//================================================================
/*! UART constructor

  uart1 = UART.new( id, *params )	# id = 1 or 2
*/
static void c_uart_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG( unit );

  // get UART unit num.
  int ch = 2;
  if( argc >= 1 && mrbc_type(v[1]) == MRBC_TT_INTEGER ) {
    ch = mrbc_integer(v[1]);
  }
  if( MRBC_KW_ISVALID(unit) ) {
    if( mrbc_type(unit) != MRBC_TT_INTEGER ) goto ERROR_RETURN;
    ch = mrbc_integer(unit);
  }
  if( ch < 1 || ch > NUM_UART_UNIT ) goto ERROR_RETURN;

  // allocate instance with UART_HANDLE table pointer.
  mrbc_value val = mrbc_instance_new(vm, v[0].cls, sizeof(UART_HANDLE *));
  *(UART_HANDLE**)(val.instance->data) = &uart_handle_[ch-1];

  // process other parameters
  v[0] = val;
  c_uart_setmode( vm, v, argc );
  goto RETURN;


 ERROR_RETURN:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "UART initialize.");

 RETURN:
  MRBC_KW_DELETE( unit );
}


//================================================================
/*! set mode

  uart1.setmode( *params )
*/
static void c_uart_setmode(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG( baudrate, baud, data_bits, stop_bits, parity, flow_control, txd_pin, rxd_pin, rts_pin, cts_pin );
  if( !MRBC_KW_END() ) goto RETURN;

  UART_HANDLE *hndl = *(UART_HANDLE **)v->instance->data;
  int baud_rate = -1;
  int flag_pin_change = 0;
  PIN_HANDLE now_txd_pin = hndl->txd_pin;

  if( MRBC_KW_ISVALID(baudrate) ) baud_rate = mrbc_integer(baudrate);
  if( MRBC_KW_ISVALID(baud) ) baud_rate = mrbc_integer(baud);
  if( MRBC_KW_ISVALID(data_bits) ) goto ERROR_NOT_IMPLEMENTED;
  if( !MRBC_KW_ISVALID(stop_bits) ) stop_bits = mrbc_integer_value(-1);
  if( !MRBC_KW_ISVALID(parity) ) parity = mrbc_integer_value(-1);
  if( MRBC_KW_ISVALID(flow_control) ) goto ERROR_NOT_IMPLEMENTED;
  if( MRBC_KW_ISVALID(txd_pin) ) {
    if( set_pin_handle( &(hndl->txd_pin), &txd_pin ) != 0 ) goto ERROR_ARGUMENT;
    flag_pin_change = 1;
  }
  if( MRBC_KW_ISVALID(rxd_pin) ) {
    if( set_pin_handle( &(hndl->rxd_pin), &rxd_pin ) != 0 ) goto ERROR_ARGUMENT;
  }
  if( MRBC_KW_ISVALID(rts_pin) ) goto ERROR_NOT_IMPLEMENTED;
  if( MRBC_KW_ISVALID(cts_pin) ) goto ERROR_NOT_IMPLEMENTED;

  // set to UART
  uart_disable( hndl );
  uart_setmode( hndl, baud_rate, parity.i, stop_bits.i );
  if( flag_pin_change ) {
    RPxnR( now_txd_pin.port, now_txd_pin.num ) = 0;	// release TxD pin.
    if( uart_assign_pin( hndl ) < 0 ) goto ERROR_ARGUMENT;
  }
  uart_enable( hndl );
  goto RETURN;


 ERROR_NOT_IMPLEMENTED:
  mrbc_raise(vm, MRBC_CLASS(NotImplementedError), 0);
  goto RETURN;

 ERROR_ARGUMENT:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), 0);

 RETURN:
  MRBC_KW_DELETE( baudrate, baud, data_bits, stop_bits, parity, flow_control, txd_pin, rxd_pin, rts_pin, cts_pin );
}


//================================================================
/*! read

  s = uart1.read(n)

  @param  n		Number of bytes receive.
  @return String	Received data.
*/
static void c_uart_read(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *hndl = *(UART_HANDLE **)v->instance->data;
  mrbc_int_t read_bytes;

  if( mrbc_type(v[1]) == MRBC_TT_INTEGER ) {
    read_bytes = mrbc_integer(v[1]);
  } else {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), 0);
    return;
  }

  if( uart_is_rx_overflow( hndl ) ) {
    mrbc_raise(vm, 0, "UART Rx buffer overflow. resetting.");
    uart_clear_rx_buffer( hndl );
    return;
  }

  mrbc_value ret = mrbc_string_new(vm, 0, read_bytes);
  char *buf = mrbc_string_cstr(&ret);
  if( !buf ) {
    SET_RETURN(mrbc_nil_value());
    return;
  }

  while( read_bytes > 0 ) {
    mrbc_int_t n = uart_bytes_available(hndl);
    if( n == 0 ) {
      Nop(); Nop(); Nop(); Nop();
      continue;
    }

    if( n > read_bytes ) n = read_bytes;
    uart_read( hndl, buf, n );

    buf += n;
    read_bytes -= n;
  }
  *buf = 0;

  SET_RETURN(ret);
}


//================================================================
/*! write

  uart1.write(s)

  @param  s	  Write data.
*/
static void c_uart_write(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *hndl = *(UART_HANDLE **)v->instance->data;

  if( v[1].tt == MRBC_TT_STRING ) {
    int n = uart_write( hndl, mrbc_string_cstr(&v[1]), mrbc_string_size(&v[1]) );
    SET_INT_RETURN(n);
  }
  else {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), 0);
  }
}


//================================================================
/*! gets

  s = uart1.gets()

  @return String	Received string.
*/
static void c_uart_gets(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *hndl = *(UART_HANDLE **)v->instance->data;

  int len;
  while( 1 ) {
    len = uart_can_read_line(hndl);
    if( len > 0 ) break;
    if( len < 0 ) {
      mrbc_raise(vm, 0, "UART Rx buffer overflow. resetting.");
      uart_clear_rx_buffer( hndl );
      return;
    }

    Nop(); Nop(); Nop(); Nop();
  }

  mrbc_value ret = mrbc_string_new(vm, 0, len);
  char *buf = mrbc_string_cstr(&ret);
  if( !buf ) {
    SET_RETURN(mrbc_nil_value());
    return;
  }

  uart_read( hndl, buf, len );
  *(buf + len) = 0;

  SET_RETURN(ret);
}


//================================================================
/*! write string with LF

  uart1.puts(s)

  @param  s	  Write data.
*/
static void c_uart_puts(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *hndl = *(UART_HANDLE **)v->instance->data;

  if( v[1].tt == MRBC_TT_STRING ) {
    const char *s = mrbc_string_cstr(&v[1]);
    int len = mrbc_string_size(&v[1]);

    uart_write( hndl, s, len );
    if( len == 0 || s[len-1] != '\n' ) {
      uart_write( hndl, UART_NL, strlen(UART_NL) );
    }
    SET_NIL_RETURN();
  }
  else {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), 0);
  }
}


//================================================================
/*! Returns the number of incoming bytes that are waiting to be read.

  uart1.bytes_available()

  @return Integer	incomming bytes
*/
static void c_uart_bytes_available(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *hndl = *(UART_HANDLE **)v->instance->data;

  SET_INT_RETURN( uart_bytes_available( hndl ) );
}


//================================================================
/*! Returns the number of bytes that are waiting to be written.

  uart1.bytes_to_write()

  @return  Integer
*/
static void c_uart_bytes_to_write(mrbc_vm *vm, mrbc_value v[], int argc)
{
  // always zero return because no write buffer.
  SET_INT_RETURN( 0 );
}


//================================================================
/*! Returns true if a line of data can be read.

  uart1.can_read_line()

  @return Boolean
*/
static void c_uart_can_read_line(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *hndl = *(UART_HANDLE **)v->instance->data;

  int len = uart_can_read_line(hndl);
  if( len < 0 ) {
    mrbc_raise(vm, 0, "UART Rx buffer overflow. resetting.");
    uart_clear_rx_buffer( hndl );
    return;
  }

  SET_BOOL_RETURN( len );
}


//================================================================
/*! flush tx buffer.

  uart1.flush()
*/
static void c_uart_flush(mrbc_vm *vm, mrbc_value v[], int argc)
{
  // nothing to do.
}


//================================================================
/*! clear tx buffer

  uart1.clear_tx_buffer()
*/
static void c_uart_clear_tx_buffer(mrbc_vm *vm, mrbc_value v[], int argc)
{
  // nothing to do.
}


//================================================================
/*! clear rx buffer

  uart1.clear_rx_buffer()
*/
static void c_uart_clear_rx_buffer(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *hndl = *(UART_HANDLE **)v->instance->data;
  uart_clear_rx_buffer( hndl );
}


//================================================================
/*! send break signal.

  uart1.clear_rx_buffer()
*/
static void c_uart_send_break(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *hndl = *(UART_HANDLE **)v->instance->data;

  while( (UxSTA(hndl->unit_num) & _U1STA_TRMT_MASK) == 0 )
    ;
  UxSTASET(hndl->unit_num) = _U1STA_UTXBRK_MASK;
  UxTXREG(hndl->unit_num) = 0x00;		// dummy data.

  while( (UxSTA(hndl->unit_num) & _U1STA_UTXBRK_MASK) )
    ;
}


//================================================================
/*! initialize
*/
void mrbc_init_class_uart(void)
{
  // define class and methods.
  mrbc_class *uart = mrbc_define_class(0, "UART", 0);

  mrbc_define_method(0, uart, "new",		c_uart_new);
  mrbc_define_method(0, uart, "setmode",	c_uart_setmode);
  mrbc_define_method(0, uart, "read",		c_uart_read);
  mrbc_define_method(0, uart, "write",		c_uart_write);
  mrbc_define_method(0, uart, "gets",		c_uart_gets);
  mrbc_define_method(0, uart, "puts",		c_uart_puts);
  mrbc_define_method(0, uart, "bytes_available",c_uart_bytes_available);
  mrbc_define_method(0, uart, "bytes_to_write",	c_uart_bytes_to_write);
  mrbc_define_method(0, uart, "can_read_line",	c_uart_can_read_line);
  mrbc_define_method(0, uart, "flush",		c_uart_flush);
  mrbc_define_method(0, uart, "clear_rx_buffer",c_uart_clear_rx_buffer);
  mrbc_define_method(0, uart, "clear_tx_buffer",c_uart_clear_tx_buffer);
  mrbc_define_method(0, uart, "send_break",	c_uart_send_break);

  mrbc_set_class_const(uart, mrbc_str_to_symid("NONE"), &mrbc_integer_value(UART_NONE));
  mrbc_set_class_const(uart, mrbc_str_to_symid("ODD"), &mrbc_integer_value(UART_ODD));
  mrbc_set_class_const(uart, mrbc_str_to_symid("EVEN"), &mrbc_integer_value(UART_EVEN));
  mrbc_set_class_const(uart, mrbc_str_to_symid("RTSCTS"), &mrbc_integer_value(UART_RTSCTS));
}
