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

uint32_t adc_pin[8][2] = {{0x10000, 0},
                          {0x20000, 1},
                          {0x1, 2},
                          {0x2, 3},
                          {0x4, 4},
                          {0x8, 5},
                          {0x4000, 10},
                          {0x8000, 9}};

void adc_init() {
    AD1CON1 = 0x80E0;
    AD1CON2 = 0x0;
    AD1CON3 = 0x100;
    AD1CSSL = 0x0;
    AD1CHS = 0x9<<16;
}

/* ============================= mruby/c codes ============================= */

void ADC1_Start(mrb_vm *vm, mrb_value *v, int argc){
   AD1CON1bits.SAMP = 1;
}

void ADC1_Stop(mrb_vm *vm, mrb_value *v, int argc){
   AD1CON1bits.SAMP = 0;
}

void ADC1_ConversionResultGet(mrb_vm *vm, mrb_value *v, int argc){
    SET_FLOAT_RETURN(3.3 * ADC1BUF0 / 1023);
}

void ADC1_ChannelSelect(mrb_vm *vm, mrb_value *v, int argc){
    int num = GET_INT_ARG(1);
    TRISB |= adc_pin[num][0];
    TRISA |= (adc_pin[num][0] >> 16);
    ANSELB |= adc_pin[num][0];
    ANSELA |= (adc_pin[num][0] >> 16);
    AD1CHS = (adc_pin[num][1] << 16);
}

void mrbc_init_class_adc(struct VM *vm){
    mrb_class *adc;
    adc = mrbc_define_class(0, "ADC",	mrbc_class_object);
    mrbc_define_method(0, adc, "start", ADC1_Start);
    mrbc_define_method(0, adc, "read", ADC1_ConversionResultGet);
    mrbc_define_method(0, adc, "stop", ADC1_Stop);
    mrbc_define_method(0, adc, "ch", ADC1_ChannelSelect);
}