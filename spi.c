/* ************************************************************************** */
/** SPI

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    spi.c

  @Summary
    SPI processing

  @Description
    mruby/c function army
 */
/* ************************************************************************** */

#include "pic32mx.h"
#include "gpio.h"
#include "mrubyc.h"

/* ================================ C codes ================================ */

/*! SPI management data
*/
typedef struct SPI_HANDLE {
  PIN_HANDLE sdi_pin;
  PIN_HANDLE sdo_pin;
  PIN_HANDLE sck_pin;
  uint8_t flag_in_use;
  uint8_t unit_num;	// 1..NUM_SPI_UNIT
} SPI_HANDLE;

#if defined(__32MX170F256B__) || defined(__PIC32MX170F256B__)
/*!
  SPI handle table with default pin assign.
*/
static SPI_HANDLE spi_handle_[NUM_SPI_UNIT] = {
  { .unit_num = 1,
    .sdi_pin = {2, 5},	// B5
    .sdo_pin = {2, 6},	// B6
    .sck_pin = {2,14},	// B14
  },
  { .unit_num = 2,
    .sdi_pin = {1, 2},	// A2
    .sdo_pin = {2,13},	// B13
    .sck_pin = {2,15},	// B15
  },
};

/*!
  SPI SDI pin setting table
*/
static const uint8_t SPI_SDI_PINS[NUM_SPI_UNIT][5] = {
  {0x11, 0x25, 0x21, 0x2b, 0x28},     // SDI1: A1, B5, B1, B11, B8
  {0x12, 0x26, 0x14, 0x2d, 0x22},     // SDI2: A2, B6, A4, B13, B2
};

/*!
  SPI SDO pin setting table
*/
static const uint8_t SPI_SDO_RPxnR[NUM_SPI_UNIT] = {
  // set value SDO1 = 0b0011, SDO2 = 0b0100
  0x03, 0x04
};
#else
#include "spi_dependent.h"
#endif


/*! assign SDI pin

  @memberof SPI_HANDLE
*/
static int spi_assign_sdi_pin( const SPI_HANDLE *hndl )
{
  gpio_setmode( &hndl->sdi_pin, GPIO_IN );

  for( int i = 0; i < sizeof(SPI_SDI_PINS)/NUM_SPI_UNIT; i++ ) {
    if( SPI_SDI_PINS[hndl->unit_num-1][i] ==
        (hndl->sdi_pin.port << 4 | hndl->sdi_pin.num) ) {
      SDIxR(hndl->unit_num) = i;
      return 0;
    }
  }

  return -1;
}


/*! assign SDO pin

  @memberof SPI_HANDLE
*/
static int spi_assign_sdo_pin( const SPI_HANDLE *hndl )
{
  gpio_setmode( &hndl->sdo_pin, GPIO_OUT );

  // set value SDO1 = 0b0011, SDO2 = 0b0100
  RPxnR( hndl->sdo_pin.port, hndl->sdo_pin.num ) = SPI_SDO_RPxnR[hndl->unit_num - 1];

  return 0;
}


/*! assign SCK pin

  @memberof SPI_HANDLE

  (note)
  SCKは、UNITごとに1カ所のみ
*/
static int spi_assign_sck_pin( const SPI_HANDLE *hndl )
{
  gpio_setmode( &hndl->sck_pin, GPIO_OUT );

  return 0;
}


/*! set SPI mode and clock

  @memberof SPI_HANDLE
  @param  mode		mode (0..3)
  @param  freq		clock frequency (Hz)
  @return		zero is no error.
*/
static int spi_setmode( const SPI_HANDLE *hndl, int mode, int32_t freq )
{
  static const uint16_t SPIxCON_CKE_CKP[] = {
    0x0100,	// 0: CKP(CPOL)=0,CKE(/CPHA)=1
    0x0000,	// 1: CKP(CPOL)=0,CKE(/CPHA)=0
    0x0140,	// 2: CKP(CPOL)=1,CKE(/CPHA)=1
    0x0040,	// 3: CKP(CPOL)=1,CKE(/CPHA)=0
  };

  if( mode >= 0 ) {
    if( mode > 3 ) return -1;
    SPIxCONCLR(hndl->unit_num) = 0x140;
    SPIxCONSET(hndl->unit_num) = SPIxCON_CKE_CKP[mode];
  }

  if( freq > 0 ) {
    uint32_t brg = (uint32_t)PBCLK / 2 / freq - 1;
    if( brg >= 0x2000 ) return -1;
    SPIxBRG(hndl->unit_num) = brg;
  }

  return 0;
}


/*! data transfer

  @memberof SPI_HANDLE
  @param  send_buf	send data
  @param  send_size	send data size (bytes)
  @param  recv_buf	receive data buffer (or NULL)
  @param  recv_size	receive data size (or zero)
  @param  flag_include	is received data include the data received when state o ftransmission?
  @return
*/
static int spi_transfer( const SPI_HANDLE *hndl,
			 const void *send_buf, int send_size,
                         void *recv_buf, int recv_size, int flag_include )
{
  int unit = hndl->unit_num;
  const uint8_t *p_send = send_buf;
  uint8_t *p_recv = recv_buf;

  // force empty FIFO.
  while( !(SPIxSTAT(unit) & _SPI1STAT_SPITBE_MASK)) {
  }
  while( !(SPIxSTAT(unit) & _SPI1STAT_SPIRBE_MASK)) {
    int dummy = SPIxBUF(unit);
    (void)dummy;	// avoid warning.
  }

  // send data state.
  int s_count = send_size;
  int r_count = send_size;
  while( r_count > 0 ) {
    // write data to FIFO
    if( s_count > 0 && !(SPIxSTAT(unit) & _SPI1STAT_SPITBF_MASK)) {
      SPIxBUF(unit) = *p_send++;
      s_count--;
    }

    // read data from FIFO
    while( !(SPIxSTAT(unit) & _SPI1STAT_SPIRBE_MASK)) {
      uint8_t data = SPIxBUF(unit);
      if( p_recv && flag_include ) *p_recv++ = data;
      r_count--;
    }
  }

  // receive data state.
  r_count = recv_size;
  if( flag_include ) r_count -= send_size;
  s_count = r_count;
  while( r_count > 0 ) {
    // write dummy data to FIFO
    if( s_count > 0 && !(SPIxSTAT(unit) & _SPI1STAT_SPITBF_MASK)) {
      SPIxBUF(unit) = 0;
      s_count--;
    }

    // read data from FIFO
    while( !(SPIxSTAT(unit) & _SPI1STAT_SPIRBE_MASK)) {
      *p_recv++ = SPIxBUF(unit);
      r_count--;
    }
  }

  return 0;
}


/* ============================= mruby/c codes ============================= */
static void c_spi_setmode(mrbc_vm *vm, mrbc_value v[], int argc);

/*! write mrbc value to spi bus.
 */
static int spi_trans_mrbc_value( const SPI_HANDLE *hndl, const mrbc_value *v,
				 mrbc_value *ret )
{
  switch( v->tt ) {
  case MRBC_TT_INTEGER: {
    uint8_t data = mrbc_fixnum(*v);
    uint8_t recv;
    spi_transfer( hndl, &data, 1, &recv, 1, 1 );
    if( ret ) {
      mrbc_string_append_cbuf( ret, &recv, 1 );
    }
  } break;

  case MRBC_TT_ARRAY:
    for( int i = 0; i < mrbc_array_size(v); i++ ) {
      mrbc_value v1 = mrbc_array_get(v, i);
      spi_trans_mrbc_value( hndl, &v1, ret );
    }
    break;

  case MRBC_TT_STRING: {
    int len1 = mrbc_string_size(v);
    char *recv = NULL;
    if( ret ) {
      int len2 = mrbc_string_size(ret);
      if( mrbc_string_append_cbuf( ret, NULL, len1 ) != 0 ) break;
      recv = mrbc_string_cstr(ret) + len2;
    }
    spi_transfer( hndl, mrbc_string_cstr(v), len1, recv, len1, 1 );
  } break;

  default:
    return -1;
  }

  return 0;
}


/*! SPI constructor

  spi = SPI.new()   # all default.
                    # mode0 1MHz SPI1 SDI=B5 SDO=B6 SCK=B14

  spi = SPI.new( params )
                    # unit: 1 or 2
                    # mode: 0..3
		    # frequency: 9770..5000000 (9.77kHz - 5MHz)
		    # sdi_pin: SPI1: A1 B1 B5 B8 B11
		    #          SPI2: A2 B2 B6 B13
		    # sdo_pin:       A2 A4 B2 B6 B13
  (note)
    SCK: SPI1: B14
         SPI2: B15
*/
static void c_spi_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG( unit );

  // get unit num.
  int unit_num = 1;
  if( argc >= 1 && mrbc_type(v[1]) == MRBC_TT_INTEGER ) {
    unit_num = mrbc_integer(v[1]);
  }
  if( MRBC_KW_ISVALID(unit) ) {
    if( mrbc_type(unit) != MRBC_TT_INTEGER ) goto ERROR_RETURN;
    unit_num = mrbc_integer(unit);
  }
  if( unit_num < 1 || unit_num > 2 ) goto ERROR_RETURN;

  // allocate instance with SPI_HANDLE table pointer.
  mrbc_value val = mrbc_instance_new(vm, v[0].cls, sizeof(SPI_HANDLE *));
  *(SPI_HANDLE**)(val.instance->data) = &spi_handle_[unit_num-1];

  if( !spi_handle_[unit_num-1].flag_in_use ) {
    // set SPI default hardware settings.
    SPIxCON(unit_num) = 0x00010120;	// ENHBUF,MSTEN
    SPIxCON2(unit_num) = 0;

    spi_setmode( &spi_handle_[unit_num-1], 0, 1000000 );
    spi_assign_sdi_pin( &spi_handle_[unit_num-1] );
    spi_assign_sdo_pin( &spi_handle_[unit_num-1] );
    spi_assign_sck_pin( &spi_handle_[unit_num-1] );
  }

  v[0] = val;
  c_spi_setmode( vm, v, argc );

  if( !spi_handle_[unit_num-1].flag_in_use ) {
    spi_handle_[unit_num-1].flag_in_use = 1;
    SPIxSTAT(unit_num) = 0;
    SPIxCONSET(unit_num) = 0x8000;	// ON
  }
  goto RETURN;


 ERROR_RETURN:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "SPI initialize.");

 RETURN:
  MRBC_KW_DELETE( unit );
}


/*! set mode

  spi.setmode( *params )
*/
static void c_spi_setmode(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG( frequency, mode, sdi_pin, sdo_pin );
  if( !MRBC_KW_END() ) goto RETURN;

  SPI_HANDLE *hndl = *(SPI_HANDLE **)v->instance->data;
  uint32_t freq = -1;
  int spi_mode = -1;
  PIN_HANDLE now_sdo_pin = hndl->sdo_pin;

  if( MRBC_KW_ISVALID(frequency) ) freq = mrbc_integer(frequency);
  if( MRBC_KW_ISVALID(mode) ) spi_mode = mrbc_integer(mode);
  if( MRBC_KW_ISVALID(sdi_pin) &&
      !set_pin_handle( &hndl->sdi_pin, &sdi_pin ) ) goto ERROR_ARGUMENT;
  if( MRBC_KW_ISVALID(sdo_pin) &&
      !set_pin_handle( &hndl->sdo_pin, &sdo_pin ) ) goto ERROR_ARGUMENT;

  // set SPI hardware settings.
  if( spi_setmode( hndl, spi_mode, freq ) != 0 ) goto ERROR_ARGUMENT;
  if( MRBC_KW_ISVALID(sdi_pin) &&
      !spi_assign_sdi_pin( hndl ) ) goto ERROR_ARGUMENT;

  if( MRBC_KW_ISVALID(sdo_pin) ) {
    RPxnR( now_sdo_pin.port, now_sdo_pin.num ) = 0;	// release SDO pin.
    spi_assign_sdo_pin( hndl );
  }
  goto RETURN;


 ERROR_ARGUMENT:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), 0);

 RETURN:
  MRBC_KW_DELETE( frequency, mode, sdi_pin, sdo_pin );
}


/*! SPI read

  s = spi.read(read_bytes)
  @param  read_bytes	Number of bytes receive.
  @return String	Received data.
*/
static void c_spi_read(mrbc_vm *vm, mrbc_value v[], int argc)
{
  SPI_HANDLE *hndl = *(SPI_HANDLE **)v->instance->data;

  if( mrbc_type(v[1]) != MRBC_TT_INTEGER ) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), 0);
    return;
  }

  mrbc_int_t read_bytes = mrbc_integer(v[1]);
  mrbc_value ret = mrbc_string_new(vm, 0, read_bytes);
  char *recv = mrbc_string_cstr(&ret);

  spi_transfer( hndl, 0, 0, recv, read_bytes, 0 );
  recv[read_bytes] = 0;

  SET_RETURN(ret);
}


/*! SPI write

  spi.write( str )
  spi.write( d1, d2, ...)
  spi.write( [d1, d2,...] )
*/
static void c_spi_write(mrbc_vm *vm, mrbc_value v[], int argc)
{
  SPI_HANDLE *hndl = *(SPI_HANDLE **)v->instance->data;

  for( int i = 1; i <= argc; i++ ) {
    spi_trans_mrbc_value( hndl, &v[i], NULL );
  }

  SET_NIL_RETURN();
}


/*! SPI transfer

  s = spi.transfer( out_data, additional_read_bytes = 0 )
*/
static void c_spi_transfer(mrbc_vm *vm, mrbc_value v[], int argc)
{
  SPI_HANDLE *hndl = *(SPI_HANDLE **)v->instance->data;
  mrbc_value ret = mrbc_string_new(vm, NULL, 0);

  if( argc == 0 ) goto RETURN;
  mrbc_int_t read_bytes = (argc >= 2) ? mrbc_integer(v[2]) : 0;

  spi_trans_mrbc_value( hndl, &v[1], &ret );

  if( read_bytes > 0 ) {
    int len = mrbc_string_size(&ret);
    if( mrbc_string_append_cbuf( &ret, NULL, read_bytes ) != 0 ) goto RETURN;
    char *recv = mrbc_string_cstr(&ret) + len;

    spi_transfer( hndl, 0, 0, recv, read_bytes, 0 );
  }

 RETURN:
  SET_RETURN(ret);
}


void mrbc_init_class_spi(struct VM *vm)
{
  mrbc_class *spi = mrbc_define_class(0, "SPI", 0);

  mrbc_define_method(0, spi, "new", c_spi_new);
  mrbc_define_method(0, spi, "setmode", c_spi_setmode);
  mrbc_define_method(0, spi, "read", c_spi_read);
  mrbc_define_method(0, spi, "write", c_spi_write);
  mrbc_define_method(0, spi, "transfer", c_spi_transfer);
}
