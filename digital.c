/* ************************************************************************** */
/** DIGITAL

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    digital.c

  @Summary
    Digital & PWM processing

  @Description
    mruby/c function army
 */
/* ************************************************************************** */

#include "digital.h"

/* ================================ C codes ================================ */

GPIO_HANDLE gpioh[21];
PWM_HANDLE pwmh[21];

void set_compare(int pin, uint32_t param){
    switch(out_compare[pin]){
        case 1:
            OC1RS = param;
            break;
        case 2:
            OC2RS = param;
            break;
        case 3:
            OC3RS = param;
            break;
        case 4:
            OC4RS = param;
            break;
        case 5:
            OC5RS = param;
            break;
    }
}

/* ============================= mruby/c codes ============================= */

static void c_leds(mrb_vm *vm, mrb_value *v, int argc) {
    int led = GET_INT_ARG(1);
    PORTAbits.RA0 = led & 0x01;
    PORTAbits.RA1 = (led & 0x02)>>1;
    PORTBbits.RB0 = (led & 0x04)>>2;
    PORTBbits.RB1 = (led & 0x08)>>3;
}

static void c_sw(mrb_vm *vm, mrb_value *v, int argc) {
    SET_INT_RETURN(PORTBbits.RB7);
}

static void c_gpio_new(mrb_vm *vm, mrb_value *v, int argc) {
    *v = mrbc_instance_new(vm, v->cls, sizeof(GPIO_HANDLE *));
    *((GPIO_HANDLE **)v->instance->data) = &gpioh[GET_INT_ARG(1)];
    GPIO_HANDLE *handle = *(GPIO_HANDLE **)v->instance->data;
    handle->pin_num = GET_INT_ARG(1);
}

static void c_gpio_write(mrb_vm *vm, mrb_value *v, int argc) {
    GPIO_HANDLE *handle = *(GPIO_HANDLE **)v->instance->data;
    int pin = handle->pin_num;
    int mode = GET_INT_ARG(1);
    if(pin < 5){
        if(mode == 0){
            PORTA &= ~(1<<pin);
        }else{
            PORTA |= (1<<pin);
        }
    }else{
        if(mode == 0){
            PORTB &= ~(1<<(pin-5));
        }else{
            PORTB |= (1<<(pin-5));
        }
    } 
}

static void c_gpio_setmode(mrb_vm *vm, mrb_value *v, int argc) {
    GPIO_HANDLE *handle = *(GPIO_HANDLE **)v->instance->data;
    int pin = handle->pin_num;
    int mode = GET_INT_ARG(2);
    if(pin < 5){
        ANSELA &= ~(1<<pin);
        if(mode == 0){
            TRISA &= ~(1<<pin);
        }else{
            TRISA |= (1<<pin);
        }
    }else{
        ANSELB &= ~(1<<pin);
        if(mode == 0){
            TRISB &= ~(1<<(pin-5));
        }else{
            TRISB |= (1<<(pin-5));
        }
    }
}

static void c_gpio_read(mrb_vm *vm, mrb_value *v, int argc) {
    GPIO_HANDLE *handle = *(GPIO_HANDLE **)v->instance->data;
    int pin = handle->pin_num;
    if(pin < 5){
        SET_INT_RETURN((PORTA>>pin)&1);
    }else{
        SET_INT_RETURN((PORTB>>(pin-5))&1);
    }
}

static void c_gpio_pull(mrb_vm *vm, mrb_value *v, int argc) {
    GPIO_HANDLE *handle = *(GPIO_HANDLE **)v->instance->data;
    int pin = handle->pin_num;
    int mode = GET_INT_ARG(2);
    if(pin < 5){
        if(mode == 0){
            CNPUA &= ~(1<<pin);
            CNPDA &= ~(1<<pin);
        }else if(mode < 0){
            CNPDA |= (1<<pin);
        }else{
            CNPUA |= (1<<pin);
        }
    }else{
        if(mode == 0){
            CNPUB &= ~(1<<(pin-5));
            CNPDB &= ~(1<<(pin-5));
        }else if(mode < 0){
            CNPDB |= (1<<(pin-5));
        }else{
            CNPUB |= (1<<(pin-5));
        }
    }
}

static void c_pwm_init(mrb_vm *vm, mrb_value *v, int argc) {
    int pin = GET_INT_ARG(1);
    *v = mrbc_instance_new(vm, v->cls, sizeof(PWM_HANDLE *));
    *((PWM_HANDLE **)v->instance->data) = &pwmh[GET_INT_ARG(1)];
    PWM_HANDLE *handle = *(PWM_HANDLE **)v->instance->data;
    handle->pin_num = pin;
    T3CONbits.TCKPS = 0;
    OC1CON = OC2CON = OC3CON = OC4CON = OC5CON = 0x800F;
    OC1R = OC2R = OC3R = OC4R = OC5R = PR3;
    if(pin < 5){
        pwm_a[pin] = 0x05;
    }else{
        pwm_b[pin-5] = 0x05;
    }
}

static void c_pwm_frequency(mrb_vm *vm, mrb_value *v, int argc) {
    PR3 = 10000000 / GET_INT_ARG(1);
    OC1R = OC2R = OC3R = OC4R = OC5R = PR3;
}

static void c_pwm_period_us(mrb_vm *vm, mrb_value *v, int argc) {
    PR3 = GET_INT_ARG(1) * 10;
    OC1R = OC2R = OC3R = OC4R = OC5R = PR3;
}

static void c_pwm_duty(mrb_vm *vm, mrb_value *v, int argc) {
    PWM_HANDLE *handle = *(PWM_HANDLE **)v->instance->data;
    int pin = handle->pin_num;
    float percent = 1023.0 / GET_INT_ARG(1);
    set_compare(pin,PR3/percent);
}

static void c_pwm_duty_us(mrb_vm *vm, mrb_value *v, int argc) {
    PWM_HANDLE *handle = *(PWM_HANDLE **)v->instance->data;
    int pin = handle->pin_num;
    uint32_t us = GET_INT_ARG(1);
    set_compare(pin,us);
}

void mrbc_init_class_onboard(struct VM *vm){
    mrbc_define_method(0, mrbc_class_object, "leds_write", c_leds);
    mrbc_define_method(0, mrbc_class_object, "sw", c_sw);
}

void mrbc_init_class_digital(struct VM *vm){
    mrb_class *gpio;
    gpio = mrbc_define_class(0, "GPIO",	mrbc_class_object);
    mrbc_define_method(0, gpio, "new", c_gpio_new);
    mrbc_define_method(0, gpio, "write", c_gpio_write);
    mrbc_define_method(0, gpio, "setmode", c_gpio_setmode);
    mrbc_define_method(0, gpio, "read", c_gpio_read);
    mrbc_define_method(0, gpio, "pull", c_gpio_pull);
}

void mrbc_init_class_pwm(struct VM *vm){
    mrb_class *pwm;
    pwm = mrbc_define_class(0, "PWM",	mrbc_class_object);
    mrbc_define_method(0, pwm, "new", c_pwm_init);
    mrbc_define_method(0, pwm, "frequency", c_pwm_frequency);
    mrbc_define_method(0, pwm, "period_us", c_pwm_period_us);
    mrbc_define_method(0, pwm, "duty", c_pwm_duty);
    mrbc_define_method(0, pwm, "duty_us", c_pwm_duty_us);
}
