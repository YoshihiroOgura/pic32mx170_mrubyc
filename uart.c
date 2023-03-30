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

#include <sys/attribs.h>
#include "uart.h"


/* ================================ C codes ================================ */
// handle table.
UART_HANDLE uart_handle_[2];

// function prototypes.
static void uart_push_rxfifo( UART_HANDLE *uh, uint8_t ch );


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
/*! UART enable or disable interrupt.
*/
static void uart_interrupt_enable( const UART_HANDLE *uh, int en_dis )
{
  switch( uh->unit_num ) {
  case 1: IEC1bits.U1RXIE = en_dis;
  case 2: IEC1bits.U2RXIE = en_dis;
  }
}


/*! assign the pin to UART

  @param  uh	UART HANDLE
  @return	if error, returns minus value.
*/
static int set_pin_to_uart( const UART_HANDLE *uh )
{
  int unit = uh->unit_num;
  int txd_p = uh->txd_pin.port;
  int txd_n = uh->txd_pin.num;
  int rxd_p = uh->rxd_pin.port;
  int rxd_n = uh->rxd_pin.num;

  if( unit < 1 || unit > 2 ) return -1;		// error return.

  /* set output (TxD) pin.
     Defaults to high level to keep high level when pin assignment is changed.
     Pin assign: DS60001168L  TABLE 11-2: OUTPUT PIN SELECTION
  */
  ANSELxCLR(txd_p) = (1 << txd_n);	// digital
  TRISxCLR(txd_p) = (1 << txd_n);		// output
  CNPUxCLR(txd_p) = (1 << txd_n);
  CNPDxCLR(txd_p) = (1 << txd_n);
  LATxSET(txd_p) = (1 << txd_n);		// set high level
  RPxnR(txd_p, txd_n) = unit;		// pin assign

  /* set input (RxD) pin.
     Pin assign: DS60001168L  TABLE 11-1: INPUT PIN SELECTION
  */
  ANSELxCLR(rxd_p) = (1 << rxd_n);	// digital
  TRISxSET(rxd_p) = (1 << rxd_n);		// input
  CNPUxCLR(rxd_p) = (1 << rxd_n);
  CNPDxCLR(rxd_p) = (1 << rxd_n);
  //                                  RPA2    B6    A4    B13   B2
  static const uint8_t U1RX_PINS[] = {0x12, 0x26, 0x14, 0x2d, 0x22};
  for( int i = 0; i < sizeof(U1RX_PINS); i++ ) {
    if( rxd_p == (U1RX_PINS[i] >> 4) && rxd_n == (U1RX_PINS[i] & 0x0f) ) {
      UxRXR(unit) = i;			// pin assign.
      return 0;
    }
  }

  return -1;
}


//================================================================
/*! initialize unit
*/
void uart_init(void)
{
  /* UART1
      TxD: RPB4(11pin) = Pin9
      RxD: RPA4(12pin) = none
  */
  uart_handle_[0].txd_pin = (PIN_HANDLE){UART1_TXD_PIN};
  uart_handle_[0].rxd_pin = (PIN_HANDLE){UART1_RXD_PIN};
  uart_handle_[0].unit_num = 1;
  uart_handle_[0].delimiter = '\n';

  // UART1 parameter.
  U1MODE = 0x0008;
  U1STA = 0x0;
  uart_set_mode( &uart_handle_[0], 19200, 0, 1 );
  set_pin_to_uart( &uart_handle_[0] );

  // interrupt level.
  IPC8bits.U1IP = 4;
  IPC8bits.U1IS = 3;

  // Enabling UART1
  uart_enable( &uart_handle_[0] );

  /* UART2
      TxD: RPB9(18pin) = Pin14
      RxD: RPB8(17pin) = Pin13
  */
  uart_handle_[1].txd_pin = (PIN_HANDLE){UART2_TXD_PIN};
  uart_handle_[1].rxd_pin = (PIN_HANDLE){UART2_RXD_PIN};
  uart_handle_[1].unit_num = 2;
  uart_handle_[1].delimiter = '\n';

  // UART2 parameter.
  U2MODE = 0x0008;
  U2STA = 0x0;
  uart_set_mode( &uart_handle_[1], 19200, 0, 1 );
  set_pin_to_uart( &uart_handle_[1] );

  // interrupt level.
  IPC9bits.U2IP = 4;
  IPC9bits.U2IS = 3;

  // Enabling UART2
  uart_enable( &uart_handle_[1] );
}


//================================================================
/*! enable uart
*/
void uart_enable( const UART_HANDLE *uh )
{
  uart_interrupt_enable( uh, 1 );
  UxSTASET(uh->unit_num) = (_U1STA_UTXEN_MASK | _U1STA_URXEN_MASK);
  UxMODESET(uh->unit_num) = _U1MODE_ON_MASK;
}


//================================================================
/*! disable uart
*/
void uart_disable( const UART_HANDLE *uh )
{
  while( (UxSTA(uh->unit_num) & _U1STA_TRMT_MASK) == 0 )
    ;
  uart_interrupt_enable( uh, 0 );
  UxMODECLR(uh->unit_num) = _U1MODE_ON_MASK;
  UxSTACLR(uh->unit_num) = (_U1STA_UTXEN_MASK | _U1STA_URXEN_MASK);
}


//================================================================
/*! set mode

  @param  baud		baud rate.
  @param  parity	0:none 1:odd 2:even
  @param  stop_bits	1 or 2
  @note いずれも設定変更しないパラメータは、-1 を渡す。
*/
int uart_set_mode( const UART_HANDLE *uh, int baud, int parity, int stop_bits )
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
    UxBRG(uh->unit_num) = brg;
  }

  if( 0 <= parity && parity <= 2 ) {
    static const uint8_t pdsel[] = { 0, 2, 1 };

    UxMODECLR(uh->unit_num) = _U1MODE_PDSEL_MASK;
    UxMODESET(uh->unit_num) = (pdsel[parity] << _U1MODE_PDSEL_POSITION);
  }

  switch(stop_bits) {
  case 1:
    UxMODECLR(uh->unit_num) = (1 << _U1MODE_STSEL_POSITION);
    break;
  case 2:
    UxMODESET(uh->unit_num) = (1 << _U1MODE_STSEL_POSITION);
    break;
  }

  return 0;
}


//================================================================
/*! Clear receive buffer.

  @memberof UART_HANDLE
  @param  uh		Pointer of UART_HANDLE.
*/
void uart_clear_rx_buffer( UART_HANDLE *uh )
{
  uart_interrupt_enable( uh, 0 );	// Disable Rx interrupt

  while( UxSTA(uh->unit_num) & _U1STA_URXDA_MASK ) {
    volatile uint8_t ch = UxRXREG(uh->unit_num); (void)ch;
  }
  uh->rx_rd = 0;
  uh->rx_wr = 0;
  uh->rx_overflow = 0;

  uart_interrupt_enable( uh, 1 );	// Enable Rx interrupt.
}


//================================================================
/*! Send out binary data.

  @memberof UART_HANDLE
  @param  uh            Pointer of UART_HANDLE.
  @param  buffer        Pointer of buffer.
  @param  size          Size of buffer.
  @return               Size of transmitted.
*/
int uart_write( UART_HANDLE *uh, const void *buffer, int size )
{
  const uint8_t *p = (const uint8_t *)buffer;
  int n = size;

  while( n > 0 ) {
    // TX-FIFOに空きができるまで待つ。
    while( UxSTA(uh->unit_num) & _U1STA_UTXBF_MASK ) {
      Nop(); Nop(); Nop(); Nop();
    }
    UxTXREG(uh->unit_num) = *p++;
    n--;
  }

  return size;
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
int uart_read( UART_HANDLE *uh, void *buffer, int size )
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
    if( mrbc_type(v[1]) != MRBC_TT_INTEGER ) goto ERROR_PARAM;
    ch = mrbc_fixnum(v[1]);
  }
  if( argc >= 2 ) {
    if( mrbc_type(v[2]) != MRBC_TT_INTEGER ) goto ERROR_PARAM;
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
  case 1: *((UART_HANDLE**)val.instance->data) = &uart_handle_[0]; break;
  case 2: *((UART_HANDLE**)val.instance->data) = &uart_handle_[1]; break;
  default: goto ERROR_PARAM;
  }

  // set baudrate.
  if( baud > 0 ) {
    UART_HANDLE *uh = *(UART_HANDLE **)val.instance->data;
    uart_disable( uh );
    uart_set_mode( uh, baud, -1, -1 );
    uart_enable( uh );
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
static void c_uart_set_mode(mrbc_vm *vm, mrbc_value v[], int argc)
{
  UART_HANDLE *uh = *(UART_HANDLE **)v->instance->data;
  int baud = -1;
  int parity = -1;
  int stop_bits = -1;
  int flag_txd_rxd = 0;
  PIN_HANDLE now_txd_pin = uh->txd_pin;

  if( argc != 1 ) goto ERROR_PARAM;
  if( v[1].tt != MRBC_TT_HASH ) goto ERROR_PARAM;

  // get parameter
  mrbc_hash_iterator ite = mrbc_hash_iterator_new( &v[1] );
  while( mrbc_hash_i_has_next(&ite) ) {
    mrbc_value *kv = mrbc_hash_i_next(&ite);
    const char *key;
    if( mrbc_type(kv[0]) == MRBC_TT_STRING ) {
      key = mrbc_string_cstr(&kv[0]);
    } else if( mrbc_type(kv[0]) == MRBC_TT_SYMBOL ) {
      key = mrbc_symbol_cstr(&kv[0]);
    } else {
      goto ERROR_PARAM;
    }

    if( strcmp("baud", key) == 0 ) {
      if( mrbc_type(kv[1]) != MRBC_TT_INTEGER ) goto ERROR_PARAM;
      baud = kv[1].i;

    } else if( strcmp("parity", key) == 0 ) {
      if( mrbc_type(kv[1]) != MRBC_TT_INTEGER ) goto ERROR_PARAM;
      parity = kv[1].i;

    } else if( strcmp("stop_bits", key) == 0 ) {
      if( mrbc_type(kv[1]) != MRBC_TT_INTEGER ) goto ERROR_PARAM;
      stop_bits = kv[1].i;

    } else if( strcmp("txd", key) == 0 ) {
      if( set_pin_handle( &(uh->txd_pin), &kv[1] ) != 0 ) goto ERROR_PARAM;
      flag_txd_rxd = 1;

    } else if( strcmp("rxd", key) == 0 ) {
      if( set_pin_handle( &(uh->rxd_pin), &kv[1] ) != 0 ) goto ERROR_PARAM;
      flag_txd_rxd = 1;

    } else {
      goto ERROR_PARAM;
    }
  }

  // set to UART
  uart_disable( uh );
  uart_set_mode( uh, baud, parity, stop_bits );
  if( flag_txd_rxd ) {
    RPxnR( now_txd_pin.port, now_txd_pin.num ) = 0;	// release TxD pin.
    if( set_pin_to_uart( uh ) < 0 ) goto ERROR_PARAM;
  }
  uart_enable( uh );
  SET_TRUE_RETURN();
  return;


 ERROR_PARAM:
  console_print("UART: set model parameter error.\n");
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
  UART_HANDLE *uh = *(UART_HANDLE **)v->instance->data;
  int need_length = GET_INT_ARG(1);

  if( uart_is_rx_overflow( uh ) ) {
    console_print("UART Rx buffer overflow. resetting.\n");
    uart_clear_rx_buffer( uh );
    goto RETURN_NIL;
  }

  if( uart_bytes_available(uh) < need_length ) {
    goto RETURN_NIL;
  }

  char *buf = mrbc_alloc( vm, need_length + 1 );
  if( !buf ) {
    goto RETURN_NIL;
  }

  int readed_length = uart_read( uh, buf, need_length );
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
  UART_HANDLE *uh = *(UART_HANDLE **)v->instance->data;
  int max_length = GET_INT_ARG(1);

  int len = uart_bytes_available(uh);
  if( len == 0 ) {
    ret = mrbc_nil_value();
    goto DONE;
  }

  if( len > max_length ) len = max_length;

  char *buf = mrbc_alloc( vm, len + 1 );
  uart_read( uh, buf, len );

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
  UART_HANDLE *uh = *(UART_HANDLE **)v->instance->data;

  switch( v[1].tt ) {
  case MRBC_TT_STRING: {
    int n = uart_write( uh,
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
  UART_HANDLE *uh = *(UART_HANDLE **)v->instance->data;

  int len = uart_can_read_line(uh);
  if( len == 0 ) goto NIL_RETURN;
  if( len <  0 ) {
    console_print("UART Rx buffer overflow. resetting.\n");
    uart_clear_rx_buffer( uh );
    goto NIL_RETURN;
  }

  char *buf = mrbc_alloc( vm, len+1 );
  if( !buf ) goto NIL_RETURN;

  uart_read( uh, buf, len );

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
  UART_HANDLE *uh = *(UART_HANDLE **)v->instance->data;
  uart_clear_rx_buffer( uh );
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
  mrbc_define_method(0, uart, "set_modem_params", c_uart_set_mode);
  mrbc_define_method(0, uart, "read",		c_uart_read);
  mrbc_define_method(0, uart, "read_nonblock",	c_uart_read_nonblock);
  mrbc_define_method(0, uart, "write",		c_uart_write);
  mrbc_define_method(0, uart, "gets",		c_uart_gets);
  mrbc_define_method(0, uart, "puts",		c_uart_write);
  mrbc_define_method(0, uart, "clear_tx_buffer", c_uart_clear_tx_buffer);
  mrbc_define_method(0, uart, "clear_rx_buffer", c_uart_clear_rx_buffer);
}
