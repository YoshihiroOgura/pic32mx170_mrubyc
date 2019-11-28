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

static void c_pin_mode(mrb_vm *vm, mrb_value *v, int argc) {
    int pin = GET_INT_ARG(1);
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

static void c_pin_write(mrb_vm *vm, mrb_value *v, int argc) {
    int pin = GET_INT_ARG(1);
    int mode = GET_INT_ARG(2);
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

static void c_pin_read(mrb_vm *vm, mrb_value *v, int argc) {
    int pin = GET_INT_ARG(1);
    if(pin < 5){
        SET_INT_RETURN((PORTA>>pin)&1);
    }else{
        SET_INT_RETURN((PORTB>>(pin-5))&1);
    }
}

static void c_pwm_init(mrb_vm *vm, mrb_value *v, int argc) {
}

static void c_pwm_cycle(mrb_vm *vm, mrb_value *v, int argc) {
    PR3 = GET_INT_ARG(1);
    T3CONbits.TCKPS = GET_INT_ARG(2);
    OC1CON = OC2CON = OC3CON = OC4CON = OC5CON = 0x800F;
    OC1R = OC2R = OC3R = OC4R = OC5R = PR3;
    OC1RS = OC2RS = OC3RS = OC4RS = OC5RS = PR3/2;
}

static void c_pwm_rate(mrb_vm *vm, mrb_value *v, int argc) {
    float percent = 100.0 / GET_INT_ARG(1);
    switch(GET_INT_ARG(2)){
        case 1:
            OC1RS = PR3/percent;
            break;
        case 2:
            OC2RS = PR3/percent;
            break;
        case 3:
            OC3RS = PR3/percent;
            break;
        case 4:
            OC4RS = PR3/percent;
            break;
        case 5:
            OC5RS = PR3/percent;
            break;
    }
}

static void c_pwm_start(mrb_vm *vm, mrb_value *v, int argc) {
    switch(GET_INT_ARG(1)){
        case 1:
            OC1CONbits.ON = 1;
            break;
        case 2:
            OC2CONbits.ON = 1;
            break;
        case 3:
            OC3CONbits.ON = 1;
            break;
        case 4:
            OC4CONbits.ON = 1;
            break;
        case 5:
            OC4CONbits.ON = 1;
            break;
    }
}

static void c_pwm_pin(mrb_vm *vm, mrb_value *v, int argc) {
    switch(GET_INT_ARG(1)){
        case 2:
            RPA2Rbits.RPA2R = 0x0006;
            break;
        case 18:
            RPB13Rbits.RPB13R = 0x0005;
            break;
        case 16:
            RPB11Rbits.RPB11R = 0x0005;
            break;
        case 12:
            RPB7Rbits.RPB7R = 0x0005;
            break;
        case 0:
            RPA2Rbits.RPA2R = 0;
            RPB13Rbits.RPB13R = 0;
            RPB11Rbits.RPB11R = 0;
            RPB7Rbits.RPB7R = 0;
            break;
    }
}

static void c_pwm_stop(mrb_vm *vm, mrb_value *v, int argc) {
    switch(GET_INT_ARG(1)){
        case 1:
            OC1CONbits.ON = 0;
            break;
        case 2:
            OC2CONbits.ON = 0;
            break;
        case 3:
            OC3CONbits.ON = 0;
            break;
        case 4:
            OC4CONbits.ON = 0;
            break;
        case 5:
            OC4CONbits.ON = 0;
            break;
    }
}

void mrbc_init_class_onboard(struct VM *vm){
    mrbc_define_method(0, mrbc_class_object, "leds_write", c_leds);
    mrbc_define_method(0, mrbc_class_object, "sw", c_sw);
}

void mrbc_init_class_digital(struct VM *vm){
    mrbc_define_method(0, mrbc_class_object, "pinMode", c_pin_mode);
    mrbc_define_method(0, mrbc_class_object, "digitalWrite", c_pin_write);
    mrbc_define_method(0, mrbc_class_object, "digitalRead", c_pin_read);
}

void mrbc_init_class_pwm(struct VM *vm){
    mrb_class *pwm;
    pwm = mrbc_define_class(0, "PWM",	mrbc_class_object);
    mrbc_define_method(0, pwm, "new", c_pwm_init);
    mrbc_define_method(0, pwm, "cycle", c_pwm_cycle);
    mrbc_define_method(0, pwm, "rate", c_pwm_rate);
    mrbc_define_method(0, pwm, "start", c_pwm_start);
    mrbc_define_method(0, pwm, "stop", c_pwm_stop);
    mrbc_define_method(0, pwm, "pin", c_pwm_pin);
}
