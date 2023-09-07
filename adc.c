/* ************************************************************************** */
/** ADC

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    adc.c

  @Summary
    ADC processing

  @Description
    mruby/c function army
 */
/* ************************************************************************** */

#include "pic32mx.h"
#include "gpio.h"
#include "mrubyc.h"

/*! ADC handle
*/
typedef struct ADC_HANDLE {
  PIN_HANDLE pin;
  uint8_t channel;
} ADC_HANDLE;

#if defined(__32MX170F256B__) || defined(__PIC32MX170F256B__)
/*
  Pin assign vs ADC channel table.
*/
static const ADC_HANDLE adc_handle_[] = {
  {{1, 0}, 0},	// AN0=RA0
  {{1, 1}, 1},	// AN1=RA1
  {{2, 0}, 2},	// AN2=RB0
  {{2, 1}, 3},	// AN3=RB1
  {{2, 2}, 4},	// AN4=RB2
  {{2, 3}, 5},	// AN5=RB3
  {{2,15}, 9},	// AN9=RB15
  {{2,14}, 10},	// AN10=RB14
  {{2,13}, 11},	// AN11=RB13
  {{2,12}, 12},	// AN12=RB12
};
#else
#include "adc_dependent.h"
#endif


/* ================================ C codes ================================ */

/* ============================= mruby/c codes ============================= */

/*! constructor

  adc1 = ADC.new( num )	# num: pin number of Rboard
  adc1 = ADC.new("A0")	# PIC origined pin assingment.

  # Rboard grove ADC terminal
  adc1 = ADC.new("B14")
  adc2 = ADC.new("B15")
*/
static void c_adc_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  if( argc != 1 ) goto ERROR_RETURN;

  PIN_HANDLE pin;
  if( set_pin_handle( &pin, &v[1] ) != 0 ) goto ERROR_RETURN;

  // find ADC channel from adc_handle_ table.
  static const int NUM = sizeof(adc_handle_)/sizeof(ADC_HANDLE);
  int i;
  for( i = 0; i < NUM; i++ ) {
    if( (adc_handle_[i].pin.port == pin.port) &&
	(adc_handle_[i].pin.num == pin.num) ) break;
  }
  if( i == NUM ) goto ERROR_RETURN;

  // allocate instance with ADC_HANDLE table pointer.
  mrbc_value val = mrbc_instance_new(vm, v[0].cls, sizeof(ADC_HANDLE *));
  *(const ADC_HANDLE **)(val.instance->data) = &adc_handle_[i];

  // set pin to analog input
  gpio_setmode( &pin, GPIO_ANALOG|GPIO_IN );

  v[0] = val;
  return;

 ERROR_RETURN:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "ADC initialize.");
}


static uint32_t read_sub(mrbc_vm *vm, mrbc_value v[], int argc)
{
  ADC_HANDLE *hndl = *(ADC_HANDLE **)v[0].instance->data;

  AD1CHSbits.CH0SA = hndl->channel;
  AD1CON1bits.SAMP = 1;
  while( !AD1CON1bits.DONE )
    ;
  AD1CON1bits.DONE = 0;

  return ADC1BUF0;
}



/*! read_voltage

  adc1.read_voltage() -> Float
*/
static void c_adc_read_voltage(mrbc_vm *vm, mrbc_value v[], int argc)
{
  uint32_t raw_val = read_sub( vm, v, argc );

  SET_FLOAT_RETURN( raw_val * 3.3 / 1023 );
}


/*! read_raw

  adc1.read_raw() -> Integer
*/
static void c_adc_read_raw(mrbc_vm *vm, mrbc_value v[], int argc)
{
  uint32_t raw_val = read_sub( vm, v, argc );

  SET_INT_RETURN( raw_val );
}


/*! Initializer
*/
void mrbc_init_class_adc(void)
{
  AD1CON1 = 0x00e0;	// SSRC=111 CLRASAM=0 ASAM=0 SAMP=0
  AD1CON2 = 0x0000;
  AD1CON3 = 0x1e09;	// SAMC=1e(60us) ADCS=09(TAD=2us)
  AD1CHS  = 0x0000;
  AD1CSSL = 0x0000;

  // Enable ADC
  AD1CON1bits.ADON = 1;

  mrbc_class *adc = mrbc_define_class(0, "ADC", 0);

  mrbc_define_method(0, adc, "new", c_adc_new);
  mrbc_define_method(0, adc, "read_voltage", c_adc_read_voltage);
  mrbc_define_method(0, adc, "read", c_adc_read_voltage);
  mrbc_define_method(0, adc, "read_raw", c_adc_read_raw);
}
