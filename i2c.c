/* ************************************************************************** */
/** I2C

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    i2c.c

  @Summary
    I2C processing

  @Description
    mruby/c function army
    use I2C2 module and frequency is 100kHz only.
 */
/* ************************************************************************** */

#include "pic32mx.h"
#include "gpio.h"
#include "mrubyc.h"

/* ================================ C codes ================================ */
#define I2CFREQ 100000	// 100kHz
#define I2C_ACK 0
#define I2C_NACK 1
#define I2C_BUS_BUSY -1
#define I2C_PARAM_ERROR -2


//================================================================
/*! Initialize I2C module.

  use I2C2 module only. SCL=B3, SDA=B2
*/
void i2c_init(void)
{
  // DS60001116G Equation 24-1: Baud Rate Generator Reload Value Calculation
  I2C2BRG = (uint16_t)(PBCLK / (2.0 * I2CFREQ) - 1 - PBCLK * 130e-9 / 2);
  I2C2CON = 0x8200;	// I2C : Enable, 100kHz
  I2C2STAT = 0x0;
}


//================================================================
/*! Send START condition

  @return	not zero if error.
*/
static int i2c_send_start(void)
{
  I2C2CONSET = (1 << _I2C2CON_SEN_POSITION);

  // Normally loops about 20 times at clock 40MHz.
  // thus, for safety 2000 loops.
  for( int i = 0; i < 2000; i++ ) {
    if( I2C2CONbits.SEN == 0 ) return 0;
  }
  return I2C_BUS_BUSY;
}


//================================================================
/*! Send RESTART condition

  @return	not zero if error.
*/
static int i2c_send_restart( void )
{
  I2C2CONSET = (1 << _I2C2CON_RSEN_POSITION);

  for( int i = 0; i < 2000; i++ ) {
    if( I2C2CONbits.RSEN == 0 ) return 0;
  }
  return I2C_BUS_BUSY;
}


//================================================================
/*! Send STOP condition

  @return	not zero if error.
*/
static int i2c_send_stop( void )
{
  I2C2CONSET = (1 << _I2C2CON_PEN_POSITION);

  for( int i = 0; i < 2000; i++ ) {
    if( I2C2CONbits.PEN == 0 ) return 0;
  }
  return I2C_BUS_BUSY;
}


//================================================================
/*! Writes a single byte.

  @param  data	data byte.
  @return	0: ACK / 1: NACK / Other: error.
*/
static int i2c_write_byte( uint8_t data )
{
  I2C2TRN = data;

  // about 100us/byte at 100kHz mode.
  // thus, for safety about 10ms.
  for( int i = 0; i < 1000; i++ ) {
    if( (I2C2STAT & (_I2C2STAT_TBF_MASK|_I2C2STAT_TRSTAT_MASK)) == 0 ) {
      return I2C2STATbits.ACKSTAT;
    }
    __delay_us( 10 );
  }

  return I2C_BUS_BUSY;
}


//================================================================
/*! Reads a single byte.

  @param  ack_nack	I2C_ACK | I2C_NACK
  @return		received data byte. or minus value if error.
*/
static int i2c_read_byte( int ack_nack )
{
  I2C2CONbits.RCEN = 1;

  for( int i = 0; I2C2CONbits.RCEN; ) {
    if( ++i > 1000 ) goto TIMEOUT_ERROR;
    __delay_us( 10 );
  }

  int ret = I2C2RCV;

  // send ack=0 or nack=1
  I2C2CONbits.ACKDT = !!ack_nack;
  I2C2CONbits.ACKEN = 1;

  for( int i = 0; i < 1000; i++ ) {
    if( I2C2CONbits.ACKEN == 0 ) return ret;	// normal return
    __delay_us(1);
  }


 TIMEOUT_ERROR:
  return I2C_BUS_BUSY;
}


//================================================================
/*! write mrbc value to i2c bus.
 */
static int i2c_write_mrbc_value( const mrbc_value *v, int *n_of_out_bytes )
{
  int ret = 0;

  switch( v->tt ) {
  case MRBC_TT_INTEGER: {
    ret = i2c_write_byte( mrbc_fixnum(*v) );
    if( ret == 0 ) (*n_of_out_bytes)++;
  } break;

  case MRBC_TT_ARRAY:
    for( int i = 0; i < mrbc_array_size(v); i++ ) {
      mrbc_value v1 = mrbc_array_get(v, i);
      ret = i2c_write_mrbc_value( &v1, n_of_out_bytes );
    }
    break;

  case MRBC_TT_STRING: {
    const char *p = mrbc_string_cstr(v);
    for( int i = 0; i < mrbc_string_size(v); i++ ) {
      ret = i2c_write_byte( *p++ );
      if( ret != 0 ) break;
      (*n_of_out_bytes)++;
    }
  } break;

  default:
    ret = I2C_PARAM_ERROR;
  }

  return ret;
}


/* ============================= mruby/c codes ============================= */

//================================================================
/*! read transaction.

  (mruby usage)
  s = i2c.read( i2c_adrs_7, read_bytes, *param )
  s.getbyte(n)  # bytes

  i2c_adrs_7 = Integer
  read_byres = Integer
  *param     = (option)

  (I2C Sequence)
  S - adrs W A - [param A...] - Sr - adrs R A - data_1 A... data_n A|N - [P]
    S : Start condition
    P : Stop condition
    Sr: Repeated start condition
    A : Ack
    N : Nack
*/
static void c_i2c_read(mrb_vm *vm, mrb_value v[], int argc)
{
  /*
    Get parameter
  */
  if( argc < 2 ) goto ERROR_PARAM;
  if( mrbc_type(v[1]) != MRBC_TT_INTEGER ) goto ERROR_PARAM;
  int i2c_adrs_7 = mrbc_integer(v[1]);

  if( mrbc_type(v[2]) != MRBC_TT_INTEGER ) goto ERROR_PARAM;
  int read_bytes = mrbc_integer(v[2]);
  if( read_bytes < 0 ) goto ERROR_PARAM;

  mrbc_value ret = mrbc_string_new(vm, 0, read_bytes);

  /*
    Start I2C communication
  */
  // start condition
  if( i2c_send_start() != 0 ) {
    mrbc_raise(vm, 0, "i2c#read: start condition failed.");
    goto STOP;
  }

  // send output data if specified.
  if( argc > 2 ) {
    // send I2C address.  address + r/w bit=0 (write).
    if( i2c_write_byte( i2c_adrs_7 << 1 ) != 0 ) {
      mrbc_raise(vm, 0, "i2c#read: write address failed.");
      goto STOP;
    }

    // send data
    for( int i = 3; i <= argc; i++ ) {
      int n_of_out_bytes = 0;
      switch( i2c_write_mrbc_value( &v[i], &n_of_out_bytes ) ) {
      case I2C_NACK:
	if( i == argc ) break;
	mrbc_raise(vm, 0, "i2c#read: NACK received.");
	goto STOP;

      case I2C_BUS_BUSY:
	mrbc_raise(vm, 0, "i2c#read: send data failed.");
	goto STOP;

      case I2C_PARAM_ERROR:
	mrbc_raise(vm, MRBC_CLASS(ArgumentError), "i2c#read: parameter error.");
	goto STOP;
      }
    }

    // send repeated start
    if( i2c_send_restart() != 0 ) {
      mrbc_raise(vm, 0, "i2c#read: repeated start condition failed.");
      goto STOP;
    }
  }

  // send I2C address.  address + r/w bit=1 (read).
  if( i2c_write_byte( (i2c_adrs_7 << 1) | 1 ) != 0 ) {
    mrbc_raise(vm, 0, "i2c#read: write address failed.");
    goto STOP;
  }

  // receive data.
  uint8_t *p = mrbc_string_cstr(&ret);
  for( int i = read_bytes-1; i >= 0; i-- ) {
    int res = i2c_read_byte( i == 0 );
    if( res < 0 ) {
      mrbc_raise(vm, 0, "i2c#read: read data failed.");
      goto STOP;
    }
    *p++ = res;
  }
  *p = 0;

  // send stop condition.
 STOP:
  i2c_send_stop();
  goto RETURN;


 ERROR_PARAM:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "i2c#read: parameter error.");

 RETURN:
  SET_RETURN(ret);
}


//================================================================
/*! write transaction.

  (mruby usage)
  i2c.write( i2c_adrs_7, write_data, ... )

  i2c_adrs_7 = Integer
  write_data = String, Integer, or Array<Integer>

  (I2C Sequence)
  S - ADRS W A - data1 A... - P

    S : Start condition
    P : Stop condition
    A : Ack
*/
static void c_i2c_write(mrb_vm *vm, mrb_value v[], int argc)
{
  /*
    Get parameter
  */
  if( argc < 1 ) goto ERROR_PARAM;
  if( mrbc_type(v[1]) != MRBC_TT_INTEGER ) goto ERROR_PARAM;
  int i2c_adrs_7 = mrbc_integer(v[1]);

  /*
    Start I2C communication
  */
  // start condition
  if( i2c_send_start() != 0 ) {
    mrbc_raise(vm, 0, "i2c#write: start condition failed.");
    goto STOP;
  }

  // send I2C address.  address + r/w bit=0 (write).
  if( i2c_write_byte( i2c_adrs_7 << 1 ) != 0 ) {
    mrbc_raise(vm, 0, "i2c#write: write address failed.");
    goto STOP;
  }

  // send data
  int n_of_out_bytes = 0;
  for( int i = 2; i <= argc; i++ ) {
    switch( i2c_write_mrbc_value( &v[i], &n_of_out_bytes ) ) {
    case I2C_NACK:
      if( i == argc ) break;
      mrbc_raise(vm, 0, "i2c#write: NACK received.");
      goto STOP;

    case I2C_BUS_BUSY:
      mrbc_raise(vm, 0, "i2c#write: send data failed.");
      goto STOP;

    case I2C_PARAM_ERROR:
      mrbc_raise(vm, MRBC_CLASS(ArgumentError), "i2c#write: parameter error.");
      goto STOP;
    }
  }

  // send stop condition.
 STOP:
  i2c_send_stop();
  goto RETURN;


 ERROR_PARAM:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "i2c#write: parameter error.");

 RETURN:
  SET_RETURN( mrbc_integer_value(n_of_out_bytes) );
}


//================================================================
/*! Send START condition

  (mruby usage)
  i2c.send_start

  @note LOW LEVEL function.
*/
static void c_i2c_send_start(mrb_vm *vm, mrb_value v[], int argc)
{
  if( i2c_send_start() != 0 ) {
    mrbc_raise(vm, 0, "start condition failed.");
  }
}


//================================================================
/*! Send RESTART condition

  (mruby usage)
  i2c.send_restart

  @note LOW LEVEL function.
*/
static void c_i2c_send_restart(mrb_vm *vm, mrb_value v[], int argc)
{
  if( i2c_send_restart() != 0 ) {
    mrbc_raise(vm, 0, "repeated start condition failed.");
  }
}


//================================================================
/*! Send STOP condition

  (mruby usage)
  i2c.send_stop

  @note LOW LEVEL function.
*/
static void c_i2c_send_stop(mrb_vm *vm, mrb_value v[], int argc)
{
  if( i2c_send_stop() != 0 ) {
    mrbc_raise(vm, 0, "stop condition failed.");
  }
}


//================================================================
/*! Low level read

  (mruby usage)
  str = i2c.raw_read( read_bytes, ack_nack = false )

  @note LOW LEVEL function.
*/
static void c_i2c_raw_read(mrb_vm *vm, mrb_value v[], int argc)
{
  /*
    Get parameter
  */
  if( argc < 1 ) goto ERROR_PARAM;
  if( mrbc_type(v[1]) != MRBC_TT_INTEGER ) goto ERROR_PARAM;
  int read_bytes = mrbc_integer(v[1]);
  if( read_bytes < 0 ) goto ERROR_PARAM;

  int ack_nack = 0;
  if( argc >= 2 ) {
    switch( mrbc_type(v[2]) ) {
    case MRBC_TT_TRUE:	ack_nack = 1;	break;
    case MRBC_TT_FALSE:	ack_nack = 0;	break;
    default:
      goto ERROR_PARAM;
    }
  }

  mrbc_value ret = mrbc_string_new(vm, 0, read_bytes);

  // receive data.
  uint8_t *p = mrbc_string_cstr(&ret);
  for( int i = read_bytes-1; i >= 0; i-- ) {
    int res = i2c_read_byte( i == 0 && ack_nack == 0);
    if( res < 0 ) {
      mrbc_raise(vm, 0, "read data failed.");
      return;
    }
    *p++ = res;
  }
  *p = 0;
  SET_RETURN(ret);
  return;

 ERROR_PARAM:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "parameter error.");
}


//================================================================
/*! Low level write

  (mruby usage)
  i2c.raw_write( *param )  # -> Integer

  @note LOW LEVEL function.
*/
static void c_i2c_raw_write(mrb_vm *vm, mrb_value v[], int argc)
{
  // send data
  int n_of_out_bytes = 0;
  for( int i = 1; i <= argc; i++ ) {
    switch( i2c_write_mrbc_value( &v[i], &n_of_out_bytes ) ) {
    case I2C_NACK:
      if( i == argc ) break;
      mrbc_raise(vm, 0, "NACK received.");
      return;

    case I2C_BUS_BUSY:
      mrbc_raise(vm, 0, "send data failed.");
      return;

    case I2C_PARAM_ERROR:
      mrbc_raise(vm, MRBC_CLASS(ArgumentError), "parameter error.");
      return;
    }
  }

  SET_RETURN( mrbc_integer_value(n_of_out_bytes) );
}


//================================================================
/*! initialize
*/
void mrbc_init_class_i2c(void)
{
  i2c_init();

  mrbc_class *i2c = mrbc_define_class(0, "I2C", 0);

  mrbc_define_method(0, i2c, "read", c_i2c_read);
  mrbc_define_method(0, i2c, "write", c_i2c_write);
  mrbc_define_method(0, i2c, "send_start", c_i2c_send_start);
  mrbc_define_method(0, i2c, "send_restart", c_i2c_send_restart);
  mrbc_define_method(0, i2c, "send_stop", c_i2c_send_stop);
  mrbc_define_method(0, i2c, "raw_read", c_i2c_raw_read);
  mrbc_define_method(0, i2c, "raw_write", c_i2c_raw_write);
}
