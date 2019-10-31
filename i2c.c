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

void i2c_init() {
    ANSELB &= 0xf3;
    TRISB |= 0xc;
    CNPUB |= 0xc;
    I2C2BRG = 50;
    I2C2CON = 0x8000;
    I2C2STAT = 0x0;
}

/* ============================= mruby/c codes ============================= */

void i2c_start() {
    I2C2CONbits.SEN = 1;
    while (I2C2CONbits.SEN) {}
}

void i2c_restart() {
    I2C2CONbits.RSEN = 1;
    while (I2C2CONbits.RSEN) {}
}

void i2c_stop() {
    I2C2CONbits.PEN = 1;
    while (I2C2CONbits.PEN) {}
}

void i2c_write(unsigned char data) {
    I2C2TRN = data;
    while (I2C2STATbits.TRSTAT) {}
}

static void c_i2c_write(mrb_vm *vm, mrb_value *v, int argc) {
    int i;
    i2c_start();
    i2c_write((GET_INT_ARG(1) << 1) | 0);
    for(i=2;i <= argc;i++){
      i2c_write(GET_INT_ARG(i));
    }
    i2c_stop();
}

static void c_i2c_read(mrb_vm *vm, mrb_value *v, int argc) {
    int addr = GET_INT_ARG(1);
    int data_addr = GET_INT_ARG(2);
    int size = GET_INT_ARG(3);
    i2c_start();
    i2c_write((addr << 1) | 0);
    i2c_write(data_addr);
    i2c_restart();
    i2c_write((addr << 1) | 1);
    int i;
    mrbc_value hako = mrbc_array_new(vm, size);
    for(i=0;i<size;i++){
        I2C2CONbits.RCEN = 1;
        while (I2C2CONbits.RCEN) {}
        mrbc_value v = mrb_fixnum_value(I2C2RCV);
        I2C2CONbits.ACKDT = 0;
        I2C2CONbits.ACKEN = 1;
        while(I2C2CONbits.ACKEN == 1);
        mrbc_array_set(&hako,i,&v);
    }
    i2c_stop();
    SET_RETURN(hako);
}

void mrbc_init_class_i2c(struct VM *vm){
    mrb_class *i2c;
    i2c = mrbc_define_class(0, "I2C",	mrbc_class_object);
    mrbc_define_method(0, i2c, "write",c_i2c_write);
    mrbc_define_method(0, i2c, "read", c_i2c_read);
}