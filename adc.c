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

#include "adc.h"


/* ================================ C codes ================================ */
void adc_init()
{
  AD1CON1 = 0x00e0;	// SSRC=111 CLRASAM=0 ASAM=0 SAMP=0
  AD1CON2 = 0x0000;
  AD1CON3 = 0x1e09;	// SAMC=1e(60us) ADCS=09(TAD=2us)
  AD1CHS  = 0x0000;
  AD1CSSL = 0x0000;

  // Enable ADC
  AD1CON1bits.ADON = 1;
}


/* ============================= mruby/c codes ============================= */

void c_adc_read(mrb_vm *vm, mrb_value *v, int argc)
{
  int16_t ch = *((int16_t *)v->instance->data);
  if( ch < 0 ) {
    SET_FLOAT_RETURN( 0 );
    return;
  }

  AD1CHSbits.CH0SA = ch;
  AD1CON1bits.SAMP = 1;
  while( !AD1CON1bits.DONE )
    ;
  AD1CON1bits.DONE = 0;

  SET_FLOAT_RETURN( ADC1BUF0 * 3.3 / 1023 );
}


void c_adc_new(mrb_vm *vm, mrb_value *v, int argc)
{
  // allocate instance with pin number memory.
  *v = mrbc_instance_new(vm, v->cls, sizeof(int16_t));
  if( !v ) return;	// raise?

  // calc ADC channel
  int pin_num;
  if( mrbc_type(v[1]) == MRBC_TT_FIXNUM ) {
    pin_num = mrbc_fixnum(v[1]);
  } else {
    pin_num = -1;
  }

  static const int8_t PIN_NUM_VS_ADC_CHANNEL[] = {
    0,1,-1,-1,-1,2,3,4,5,-1,-1,-1,-1,-1,-1,-1,-1,12,11,10,9
  };
  if( pin_num < 0 || pin_num >= sizeof(PIN_NUM_VS_ADC_CHANNEL) ) {
    *(int16_t *)(v->instance->data) = -1;
    console_printf("ADC: Illegal pin number. (pin = 0..20)\n");
    return;
  }
  if( (*(int16_t *)(v->instance->data) = PIN_NUM_VS_ADC_CHANNEL[pin_num]) < 0 ) {
    console_printf("ADC: Pin %d cannot be analog input.\n", pin_num );
    return;
  }

  // set pin to analog input.
  if( pin_num < 2 ) {
    ANSELASET = (1 << pin_num);
    TRISASET = (1 << pin_num);
  } else {
    ANSELBSET = (1 << (pin_num - 5));
    TRISBSET = (1 << (pin_num - 5));
  }
}


void mrbc_init_class_adc(struct VM *vm)
{
    mrb_class *adc;
    adc = mrbc_define_class(0, "ADC", 0);
    mrbc_define_method(0, adc, "new", c_adc_new);
    mrbc_define_method(0, adc, "read", c_adc_read);
    mrbc_define_method(0, adc, "read_v", c_adc_read);
}
