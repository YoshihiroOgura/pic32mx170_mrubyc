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
  if( mode & GPIO_PULL_UP ) CNPUxSET(pin->port) = (1 << pin->num);
  if( mode & GPIO_PULL_DOWN ) CNPDxSET(pin->port) = (1 << pin->num);
  if( mode & GPIO_OPEN_DRAIN ) ODCxSET(pin->port) = (1 << pin->num);

  return 0;
}


/* ============================= mruby/c codes ============================= */
/*! constructor

  gpio1 = GPIO.new( num )  # num = pin number of Rboard.
  gpio1 = GPIO.new("A0")   # PIC origined pin assignment.
*/
static void c_gpio_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  if( argc != 2 ) goto ERROR_RETURN;

  mrbc_value val = mrbc_instance_new( vm, v[0].cls, sizeof(PIN_HANDLE) );
  PIN_HANDLE *pin = (PIN_HANDLE*)val.instance->data;

  if( set_pin_handle( pin, &v[1] ) != 0 ) goto ERROR_RETURN;
  if( v[2].tt != MRBC_TT_INTEGER ) goto ERROR_RETURN;
  if( (mrbc_integer(v[2]) & (GPIO_IN|GPIO_OUT|GPIO_HIGH_Z)) == 0 ) goto ERROR_RETURN;
  if( gpio_setmode( pin, mrbc_integer(v[2]) ) < 0 ) goto ERROR_RETURN;

  SET_RETURN( val );
  return;

 ERROR_RETURN:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "GPIO initialize.");
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


/*! setmode

  gpio1.setmode( GPIO::PULL_UP )
*/
static void c_gpio_setmode(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PIN_HANDLE *pin = (PIN_HANDLE *)v[0].instance->data;

  if( v[1].tt != MRBC_TT_INTEGER ) return;
  gpio_setmode( pin, mrbc_integer(v[1]) );
}


/*! Initializer
*/
void mrbc_init_class_gpio( void )
{
  mrbc_class *gpio = mrbc_define_class(0, "GPIO", 0);

  mrbc_define_method(0, gpio, "new", c_gpio_new);
  mrbc_define_method(0, gpio, "read_at", c_gpio_read_at);
  mrbc_define_method(0, gpio, "high_at?", c_gpio_high_at);
  mrbc_define_method(0, gpio, "low_at?", c_gpio_low_at);
  mrbc_define_method(0, gpio, "write_at", c_gpio_write_at);

  mrbc_define_method(0, gpio, "read", c_gpio_read);
  mrbc_define_method(0, gpio, "high?", c_gpio_high);
  mrbc_define_method(0, gpio, "low?", c_gpio_low);
  mrbc_define_method(0, gpio, "write", c_gpio_write);
  mrbc_define_method(0, gpio, "setmode", c_gpio_setmode);

  mrbc_set_class_const(gpio, mrbc_str_to_symid("IN"),         &mrbc_integer_value(GPIO_IN));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("OUT"),        &mrbc_integer_value(GPIO_OUT));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("HIGH_Z"),     &mrbc_integer_value(GPIO_HIGH_Z));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("PULL_UP"),    &mrbc_integer_value(GPIO_PULL_UP));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("PULL_DOWN"),  &mrbc_integer_value(GPIO_PULL_DOWN));
  mrbc_set_class_const(gpio, mrbc_str_to_symid("OPEN_DRAIN"), &mrbc_integer_value(GPIO_OPEN_DRAIN));
}
