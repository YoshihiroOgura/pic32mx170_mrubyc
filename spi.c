/* ************************************************************************** */
/** SPI

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    spi.c

  @Summary
    SPI processing

  @Description
    mruby/c function army
 */
/* ************************************************************************** */

#include "spi.h"

/* ================================ C codes ================================ */

void spi_init() {
    ANSELB &= 0x000F;
    TRISB &= 0x13FF;
    SPI2CONbits.ON = 0;
    
    SPI2BRG = 0x3;
    SPI2CONbits.MSTEN = 1;
    SPI2CONbits.CKP = 1;
    SPI2CONbits.CKE = 0;
    SPI2CONbits.SMP = 0;
    SPI2CONbits.DISSDO = 0;
    //SPI2CONbits.MSSEN = 1;
    
    SDI2Rbits.SDI2R = 0b0001;
    RPB11Rbits.RPB11R = 0b0100;
    //SS2Rbits.SS2R = 0b0011;
    
    SPI2CONbits.ON = 1;
    PORTBbits.RB14 = 1;
}

void spi_write(char data){
    char c;
    c = SPI2BUF;
    SPI2BUF = data;
    while(SPI2STATbits.SPITBF);
    c = SPI2BUF;
}

static void c_spi_write(mrb_vm *vm, mrb_value *v, int argc) {
    char i;
    for(i=1;i <= argc;i++){
        spi_write(GET_INT_ARG(i));
    }
}
static void c_spi_read(mrb_vm *vm, mrb_value *v, int argc) {
    int size = GET_INT_ARG(1);
    unsigned char c;
    int i;
    mrbc_value hako = mrbc_array_new(vm, size);
    for(i=0;i<size;i++){
        c = SPI2BUF;
        SPI2BUF = 0;
        while(SPI2STATbits.SPITBF);
        while(!SPI2STATbits.SPIRBF);
        c = SPI2BUF;
        mrbc_value v = mrb_fixnum_value(c);
        mrbc_array_set(&hako,i,&v);
    }
    SET_RETURN(hako);
}

static void c_spi_new(mrb_vm *vm, mrb_value *v, int argc) {
    spi_init();
}
/*
static void c_spi_write(mrb_vm *vm, mrb_value *v, int argc) {
    int i;
    for(i=1;i <= argc;i++){
      spi_write(GET_INT_ARG(i));
    }
}*/
/*
static void c_spi_read(mrb_vm *vm, mrb_value *v, int argc) {
    int size = GET_INT_ARG(1);
    int i;
    mrbc_value hako = mrbc_array_new(vm, size);
    for(i=0;i<size;i++){
        mrbc_value v = mrb_fixnum_value(SPI2BUF);
        while(SPI2STATbits.SPIBUSY) ;
        mrbc_array_set(&hako,i,&v);
    }
    SET_RETURN(hako);
}*/

void mrbc_init_class_spi(struct VM *vm){
    mrb_class *spi;
    spi = mrbc_define_class(0, "SPI",	mrbc_class_object);
    mrbc_define_method(0, spi, "new",c_spi_new);
    mrbc_define_method(0, spi, "write",c_spi_write);
    mrbc_define_method(0, spi, "read",c_spi_read);
}