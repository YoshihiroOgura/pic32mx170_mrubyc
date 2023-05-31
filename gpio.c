/* ************************************************************************** */
/** GPIO

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    gpio.c

  @Summary
    GPIO class processing

  @Description
    mruby/c function army
 */
/* ************************************************************************** */

#include "pic32mx.h"
#include "gpio.h"
#include "mrubyc.h"


/* ================================ C codes ================================ */

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


/*! set (change) mode

  @param  pin	target pin.
  @param  mode	mode. Sepcified by GPIO_* constant.
  @return int	zero is no error.
*/
int gpio_setmode( const PIN_HANDLE *pin, unsigned int mode )
{
  if( mode & (GPIO_IN|GPIO_OUT|GPIO_ANALOG|GPIO_HIGH_Z) ) {
    if( mode & GPIO_ANALOG ) {
      ANSELxSET(pin->port) = (1 << pin->num);
    } else {
      ANSELxCLR(pin->port) = (1 << pin->num);
    }
    CNPUxCLR(pin->port) = (1 << pin->num);
    CNPDxCLR(pin->port) = (1 << pin->num);
    ODCxCLR(pin->port) = (1 << pin->num);
  }
  if( mode & GPIO_IN ) TRISxSET(pin->port) = (1 << pin->num);
  if( mode & GPIO_OUT ) TRISxCLR(pin->port) = (1 << pin->num);
  if( mode & GPIO_HIGH_Z ) return -1;

  if( mode & GPIO_PULL_UP ) {
    CNPDxCLR(pin->port) = (1 << pin->num);
    CNPUxSET(pin->port) = (1 << pin->num);
  }
  if( mode & GPIO_PULL_DOWN ) {
    CNPUxCLR(pin->port) = (1 << pin->num);
    CNPDxSET(pin->port) = (1 << pin->num);
  }
  if( mode & GPIO_OPEN_DRAIN ) ODCxSET(pin->port) = (1 << pin->num);

  return 0;
}


/* ============================= mruby/c codes ============================= */
/*! constructor

  gpio1 = GPIO.new(pin, GPIO::IN )
*/
static void c_gpio_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(PIN_HANDLE));
  mrbc_instance_call_initialize( vm, v, argc );
}


/*! initializer
*/
static void c_gpio_initialize(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE *pin = (PIN_HANDLE *)v[0].instance->data;

  if( argc != 2 ) goto ERROR_RETURN;
  if( set_pin_handle( pin, &v[1] ) != 0 ) goto ERROR_RETURN;
  if( (mrbc_integer(v[2]) & (GPIO_IN|GPIO_OUT|GPIO_HIGH_Z)) == 0 ) goto ERROR_RETURN;
  if( gpio_setmode( pin, mrbc_integer(v[2]) ) < 0 ) goto ERROR_RETURN;
  return;

 ERROR_RETURN:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "GPIO initialize");
}


/*! setmode

  GPIO.setmode( num, GPIO::IN )		# class method
  gpio1.setmode( GPIO::PULL_UP )	# instance method
*/
static void c_gpio_setmode(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE pin;

  if( v[0].tt == MRBC_TT_OBJECT ) goto INSTANCE_METHOD_MODE;

  /*
    Class method mode.
  */
  if( argc != 2 ) goto ERROR_RETURN;
  if( set_pin_handle( &pin, &v[1] ) != 0 ) goto ERROR_RETURN;
  if( v[2].tt != MRBC_TT_INTEGER ) goto ERROR_RETURN;
  if( gpio_setmode( &pin, mrbc_integer(v[2]) ) < 0 ) goto ERROR_RETURN;
  return;

  /*
    Instance method mode.
  */
 INSTANCE_METHOD_MODE:
  pin = *(PIN_HANDLE *)v[0].instance->data;

  if( v[1].tt != MRBC_TT_INTEGER ) goto ERROR_RETURN;
  if( gpio_setmode( &pin, mrbc_integer(v[1]) ) < 0 ) goto ERROR_RETURN;
  SET_NIL_RETURN();
  return;

 ERROR_RETURN:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "GPIO Can't setup");
}


/*! read_at -> Integer

  v1 = GPIO.read_at( 1 )          # read from pin 1.
*/
static void c_gpio_read_at(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE pin;

  if( set_pin_handle( &pin, &v[1] ) == 0 ) {
    SET_INT_RETURN( (PORTx(pin.port) >> pin.num) & 1 );
  } else {
    SET_NIL_RETURN();
  }
}


/*! high_at? -> bool

  v1 = GPIO.read_at( 1 )          # read from pin 1.
*/
static void c_gpio_high_at(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE pin;

  if( set_pin_handle( &pin, &v[1] ) == 0 ) {
    SET_BOOL_RETURN( (PORTx(pin.port) >> pin.num) & 1 );
  } else {
    SET_NIL_RETURN();
  }
}


/*! low_at? -> bool

  v1 = GPIO.read_at( 1 )          # read from pin 1.
*/
static void c_gpio_low_at(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE pin;

  if( set_pin_handle( &pin, &v[1] ) == 0 ) {
    SET_BOOL_RETURN( ~(PORTx(pin.port) >> pin.num) & 1 );
  } else {
    SET_NIL_RETURN();
  }
}


/*! write_at

  v1 = GPIO.write_at( 1, 0 )      # output zero to pin 1.
*/
static void c_gpio_write_at(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE pin;

  if( (set_pin_handle( &pin, &v[1] ) != 0) ||
      (v[2].tt != MRBC_TT_INTEGER) ) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), 0);
    return;
  }

  if( mrbc_integer(v[2]) == 0 ) {
    LATxCLR(pin.port) = (1 << pin.num);
  } else if( mrbc_integer(v[2]) == 1 ) {
    LATxSET(pin.port) = (1 << pin.num);
  } else {
    mrbc_raise(vm, MRBC_CLASS(RangeError), 0);
  }
}


/*! read

  x = gpio1.read()
*/
static void c_gpio_read(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE *pin = (PIN_HANDLE *)v[0].instance->data;

  SET_INT_RETURN( (PORTx(pin->port) >> pin->num) & 1 );
}


/*! high?

  if gpio1.high?() ...
*/
static void c_gpio_high(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE *pin = (PIN_HANDLE *)v[0].instance->data;

  SET_BOOL_RETURN( (PORTx(pin->port) >> pin->num) & 1 );
}


/*! low?

  if gpio1.low?() ...
*/
static void c_gpio_low(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE *pin = (PIN_HANDLE *)v[0].instance->data;

  SET_BOOL_RETURN( ~(PORTx(pin->port) >> pin->num) & 1 );
}


/*! write

  gpio1.write( 0 or 1 )
*/
static void c_gpio_write(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE *pin = (PIN_HANDLE *)v[0].instance->data;

  if( v[1].tt != MRBC_TT_INTEGER ) return;
  if( mrbc_integer(v[1]) == 0 ) {
    LATxCLR(pin->port) = (1 << pin->num);
  } else {
    LATxSET(pin->port) = (1 << pin->num);
  }
}


/*! Initializer
*/
void mrbc_init_class_gpio( void )
{
  mrbc_class *gpio = mrbc_define_class(0, "GPIO", 0);

  mrbc_define_method(0, gpio, "new", c_gpio_new);
  mrbc_define_method(0, gpio, "initialize", c_gpio_initialize);
  mrbc_define_method(0, gpio, "setmode", c_gpio_setmode);
  mrbc_define_method(0, gpio, "read_at", c_gpio_read_at);
  mrbc_define_method(0, gpio, "high_at?", c_gpio_high_at);
  mrbc_define_method(0, gpio, "low_at?", c_gpio_low_at);
  mrbc_define_method(0, gpio, "write_at", c_gpio_write_at);

  mrbc_define_method(0, gpio, "read", c_gpio_read);
  mrbc_define_method(0, gpio, "high?", c_gpio_high);
  mrbc_define_method(0, gpio, "low?", c_gpio_low);
  mrbc_define_method(0, gpio, "write", c_gpio_write);

  mrbc_set_class_const(gpio, mrbc_str_to_symid("IN"),         &mrbc_integer_value(GPIO_IN));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("OUT"),        &mrbc_integer_value(GPIO_OUT));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("HIGH_Z"),     &mrbc_integer_value(GPIO_HIGH_Z));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("PULL_UP"),    &mrbc_integer_value(GPIO_PULL_UP));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("PULL_DOWN"),  &mrbc_integer_value(GPIO_PULL_DOWN));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("OPEN_DRAIN"), &mrbc_integer_value(GPIO_OPEN_DRAIN));
}
