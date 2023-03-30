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
 */
/* ************************************************************************** */

#include "i2c.h"

/* ================================ C codes ================================ */

//================================================================
/*! initialize
*/
void i2c_init(void)
{
  ANSELBbits.ANSB2 = 0;
  ANSELBbits.ANSB3 = 0;
  TRISBbits.TRISB2 = 1;	// need? this is default values.
  TRISBbits.TRISB3 = 1;	// need?
  CNPUBbits.CNPUB2 = 1;
  CNPUBbits.CNPUB3 = 1;

  I2C2BRG = 0x31;	// 0x31 : see DS60001116G Table 24-2
  I2C2CON = 0x8200;	// I2C : Enable, 100kHz
  I2C2STAT = 0x0;
}


//================================================================
/*! I2C Send START condition

  @return	not zero if error.
*/
static int i2c_start(void)
{
  I2C2CONSET = (1 << _I2C2ACON_SEN_POSITION);

  // Normally loops about 20 times at clock 40MHz.
  // thus, for safety 2000 loops.
  int i;
  for( i = 0; i < 2000; i++ ) {
    if( I2C2CONbits.SEN == 0 ) return 0;
  }
  return -1;
}


//================================================================
/*! I2C Send RESTART condition

  @return	not zero if error.
*/
static int i2c_restart(void)
{
  I2C2CONSET = (1 << _I2C2ACON_RSEN_POSITION);

  int i;
  for( i = 0; i < 3000; i++ ) {
    if( I2C2CONbits.RSEN == 0 ) return 0;
  }
  return -1;
}


//================================================================
/*! I2C Send STOP condition

  @return	not zero if error.
*/
static int i2c_stop(void)
{
  I2C2CONSET = (1 << _I2C2CON_PEN_POSITION);

  int i;
  for( i = 0; i < 3000; i++ ) {
    if( I2C2CONbits.PEN == 0 ) return 0;
  }
  return -1;
}


//================================================================
/*! I2C write 1 byte.

  @param  data	data byte.
  @return	0: ACK / 1: NACK / Other: error.
*/
static int i2c_write(uint8_t data)
{
  I2C2TRN = data;

  // about 100us/byte at 100kHz mode.
  // thus, for safety about 10ms.
  int i;
  for( i = 0; i < 1000; i++ ) {
    if( (I2C2STAT & (_I2C2STAT_TBF_MASK|_I2C2STAT_TRSTAT_MASK)) == 0 ) {
      return I2C2STATbits.ACKSTAT;
    }
    __delay_us( 10 );
  }

  return -1;
}


//================================================================
/*! I2C read (receive) 1 byte.

  @param  ack_nack	ack = 0 / nack = 1
  @return		received data byte. or minus value if error.
*/
static int i2c_read( int ack_nack )
{
  I2C2CONbits.RCEN = 1;

  int i;
  for( i = 0; I2C2CONbits.RCEN; ) {
    if( ++i > 1000 ) goto TIMEOUT_ERROR;
    __delay_us( 10 );
  }

  int ret = I2C2RCV;

  // send ack=0 or nack=1
  I2C2CONbits.ACKDT = !!ack_nack;
  I2C2CONbits.ACKEN = 1;

  for( i = 0; i < 1000; i++ ) {
    if( I2C2CONbits.ACKEN == 0 ) return ret;	// normal return
    __delay_us(1);
  }

  for( i = 0; I2C2CONbits.ACKEN; ) {
    if( ++i > 1000 ) goto TIMEOUT_ERROR;
    __delay_us( 1 );
  }

  return ret;

 TIMEOUT_ERROR:
  return -1;
}


/* ============================= mruby/c codes ============================= */

//================================================================
/*! I2C write

  (mruby usage)
  i2c.write( i2c_adrs_7, write_data, ... )

  i2c_adrs_7 = Fixnum
  write_data = String, or Fixnum...

  (I2C Sequence)
  S - ADRS W A - data1 A... - P

    S : Start condition
    P : Stop condition
    A : Ack
*/
static void c_i2c_write(mrb_vm *vm, mrb_value *v, int argc)
{
  /*
    Get parameter
  */
  int i2c_adrs_7;
  int write_bytes;
  const char *write_ptr = 0;
  int res = 0;

  if( argc < 1 ) goto ERROR_PARAM;
  if( mrbc_type(v[1]) != MRBC_TT_FIXNUM ) goto ERROR_PARAM;
  i2c_adrs_7 = mrbc_fixnum(v[1]);

  if( argc >= 2 && mrbc_type(v[2]) == MRBC_TT_STRING ) {
    write_bytes = mrbc_string_size( &v[2] );
    write_ptr = mrbc_string_cstr( &v[2] );
  } else {
    write_bytes = argc - 1;
  }

  /*
    Start I2C communication
  */
  // start condition
  res = i2c_start();
  if( res != 0 ) {
    console_printf("i2c.write: start condition failed.\n");
    goto STOP;
  }

  // send I2C address.
  res = i2c_write( i2c_adrs_7 << 1 );	// address + r/w bit=0 (write).
  if( res != 0 ) {
    console_printf("i2c.write: write address failed.\n");
    goto STOP;
  }

  // send data.
  int i;
  for( i = 0; i < write_bytes; i++ ) {
    if( write_ptr ) {
      res = i2c_write( *write_ptr++ );
    } else {
      if( mrbc_type(v[i+2]) != MRBC_TT_FIXNUM ) goto ERROR_PARAM;
      res = i2c_write( mrbc_fixnum(v[i+2]) );
    }
    if( res != 0 ) {
      console_printf("i2c.write: send data failed.\n");
      goto STOP;
    }
  }

  // send stop condition.
 STOP:
  i2c_stop();
  SET_INT_RETURN( res );
  return;

 ERROR_PARAM:
  console_printf("i2c.write: parameter error.\n");
  SET_INT_RETURN(1);
}


//================================================================
/*! I2C read

  (mruby usage)
  s = i2c.read( i2c_adrs_7, read_bytes, *params )
  s.getbyte(n)  # bytes

  i2c_adrs_7 = Fixnum
  read_byres = Fixnum
  *params    = Fixnum (option)

  (I2C Sequence)
  S - ADRS W A - [params A...] - Sr - ADRS R A - data_1 A... data_n A|N - [P]
    S : Start condition
    P : Stop condition
    Sr: Repeated start condition
    A : Ack
    N : Nack
*/
static void c_i2c_read(mrb_vm *vm, mrb_value *v, int argc)
{
  /*
    Get parameter
  */
  int i2c_adrs_7;
  int read_bytes;
  uint8_t *buf;
  int flag_params;
  int res = 0;

  if( argc < 2 ) goto ERROR_PARAM;
  if( mrbc_type(v[1]) != MRBC_TT_FIXNUM ) goto ERROR_PARAM;
  i2c_adrs_7 = mrbc_fixnum(v[1]);

  if( mrbc_type(v[2]) != MRBC_TT_FIXNUM ) goto ERROR_PARAM;
  read_bytes = mrbc_fixnum(v[2]);
  buf = mrbc_alloc(vm, read_bytes + 1);
  if( !buf ) return;	// ENOMEM

  flag_params = (argc - 2) > 0;

  /*
    Start I2C communication
  */
  // start condition
  res = i2c_start();
  if( res != 0 ) {
    console_printf("i2c.read: start condition failed.\n");
    goto STOP;
  }

  // send params if specified.
  if( flag_params ) {
    // send I2C address.
    res = i2c_write( i2c_adrs_7 << 1 );	// address + r/w bit=0 (write).
    if( res != 0 ) {
      console_printf("i2c.read: write address failed.\n");
      goto STOP;
    }

    int i;
    for( i = 3; i <= argc; i++ ) {
      if( mrbc_type(v[i]) != MRBC_TT_FIXNUM ) goto ERROR_PARAM;
      res = i2c_write( mrbc_fixnum(v[i]) );
      if( res != 0 ) {
	console_printf("i2c.read: send data failed.\n");
	goto STOP;
      }
    }

    // send repeated start
    res = i2c_restart();
    if( res != 0 ) {
      console_printf("i2c.read: repeated start condition failed.\n");
      goto STOP;
    }
  }

  // send I2C address
  res = i2c_write( (i2c_adrs_7 << 1) | 1 );	// address + r/w bit=1 (read).
  if( res != 0 ) {
    console_printf("i2c.read: read address failed.\n");
    goto STOP;
  }

  // receive data.
  uint8_t *p = buf;
  int i;
  for( i = read_bytes-1; i >= 0; i-- ) {
    res = i2c_read( i == 0 );
    if( res < 0 ) {
      console_printf("i2c.read: read data failed.\n");
      goto STOP;
    }
    *p++ = res;
  }
  res = 0;
  *p = 0;

  // send stop condition.
 STOP:
  i2c_stop();

  // return
  if( res != 0 ) {
    SET_NIL_RETURN();
  } else {
    mrbc_value ret = mrbc_string_new_alloc(vm, buf, read_bytes);
    SET_RETURN( ret );
  }
  return;

 ERROR_PARAM:
  console_printf("i2c.read: parameter error.\n");
  SET_NIL_RETURN();
}


//================================================================
/*! I2C get status

  (mruby usage)
  s = i2c.status()
*/
static void c_i2c_status(mrb_vm *vm, mrb_value *v, int argc)
{
  SET_INT_RETURN(I2C2STAT);
}


//================================================================
/*! initialize
*/
void mrbc_init_class_i2c(struct VM *vm){
  mrbc_class *i2c;
  i2c = mrbc_define_class(0, "I2C",	mrbc_class_object);

  mrbc_define_method(0, i2c, "status", c_i2c_status);
  mrbc_define_method(0, i2c, "write", c_i2c_write);
  mrbc_define_method(0, i2c, "read", c_i2c_read);
}
