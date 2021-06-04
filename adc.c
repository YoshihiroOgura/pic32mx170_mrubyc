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
#include "delay.h"

/* ================================ C codes ================================ */
ADC_HANDLE adch[21];
uint32_t adc_pin[21] = {0,1,0,0,0,2,3,4,5,0,0,0,0,0,0,0,0,12,11,10,9};

int adc_chs(int num){
    TRISB |= (0xffff & (1<<(num-5))); 
    TRISA |= (0x1f & (1<<num));
    ANSELB |= (0xffff & (1<<(num-5)));
    ANSELA |= (0x1f & (1<<num));
    int chs;
    if(num ==0){
        chs = 0;
    }else{
        chs = adc_pin[num];
        if(chs==0){return 0;}
    }
    AD1CHS = (chs<<16);
    return 1;
}

void adc_init() {
    AD1CON1 = 0x80E0;
    AD1CON2 = 0x0;
    AD1CON3 = 0x100;
    AD1CSSL = 0x0;
}

/* ============================= mruby/c codes ============================= */

void c_adc_read(mrb_vm *vm, mrb_value *v, int argc){
    ADC_HANDLE *handle = *(ADC_HANDLE **)v->instance->data;
    int pin = handle->pin_num;
    int status = adc_chs(pin);
    if(status==0){return 0;}
    AD1CON1bits.SAMP = 1;
    __delay_us(10);
    int adc_p = ADC1BUF0;
    AD1CON1bits.SAMP = 0;
    SET_FLOAT_RETURN(adc_p);
}

void c_adc_read_v(mrb_vm *vm, mrb_value *v, int argc){
    ADC_HANDLE *handle = *(ADC_HANDLE **)v->instance->data;
    int pin = handle->pin_num;
    int status = adc_chs(pin);
    if(status==0){return 0;}
    AD1CON1bits.SAMP = 1;
    __delay_us(10);
    float adc_p = 3.3 * ADC1BUF0 / 1023;
    AD1CON1bits.SAMP = 0;
    SET_FLOAT_RETURN(adc_p);
}

void c_adc_new(mrb_vm *vm, mrb_value *v, int argc){
    *v = mrbc_instance_new(vm, v->cls, sizeof(ADC_HANDLE *));
    *((ADC_HANDLE **)v->instance->data) = &adch[GET_INT_ARG(1)];
    ADC_HANDLE *handle = *(ADC_HANDLE **)v->instance->data;
    handle->pin_num = GET_INT_ARG(1);
}

void mrbc_init_class_adc(struct VM *vm){
    mrb_class *adc;
    adc = mrbc_define_class(0, "ADC",	mrbc_class_object);
    mrbc_define_method(0, adc, "new", c_adc_new);
    mrbc_define_method(0, adc, "read", c_adc_read);
    mrbc_define_method(0, adc, "read_v", c_adc_read_v);
}