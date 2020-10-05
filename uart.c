/* ************************************************************************** */
/** UART

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    uart.c

  @Summary
    UART processing

  @Description
    mruby/c function army
 */
/* ************************************************************************** */

#include "uart.h"
#include <sys/attribs.h>

/* ================================ C codes ================================ */

char str[50];
int sp = 0;

void __ISR(_UART_2_VECTOR, IPL4AUTO) OnUartReceiving(void)
{
    IFS1bits.U2RXIF = 0; // UART1??????????

    if(U2STAbits.URXDA == 0) return; // ????????????????
    str[sp] = (char)U2RXREG; // ????????????????
    sp++;
    if(sp ==51){
        sp = 0;
    }
}

void uart_init(void){
   ANSELA = 0x0000;
   IPC8bits.U1IP = 1;
   IPC8bits.U1IS = 0;
   TRISAbits.TRISA4 = 1;
   U1RXRbits.U1RXR = 0x0002;   //RA4->UART1:U1RX;
   RPB4Rbits.RPB4R = 0x0001;   //RB4->UART1:U1TX;
   U1MODE = 0x8008;
   U1STA = 0x0;
   U1TXREG = 0x0;
   // BaudRate = 19200; Frequency = 40kHz; BRG 129; 
   U1BRG = 129;
   U1STAbits.UTXEN = 1;        //TX_enable
   U1STAbits.URXEN = 1;        //RX_enable

   //Enabling UART
   U1MODEbits.ON = 1;
}

void u_puts(char *mo, int size){
    int i = 0;
    if(size == 0){
        size = strlen(mo);
    }
    while (U1STAbits.TRMT == 0);
    while(i<size){
        if(mo[i]==0){break;}
        while (U1STAbits.TRMT == 0);
        U1TXREG= mo[i];
        i++;
    }
}

int u_read(char *addr){
    int i = 0;
    while(1){
      IFS1bits.U1RXIF = 0;
      if(U1STAbits.URXDA){
          addr[i] = U1RXREG;
          if(addr[i] == 0x0a){
             break;
          }
          i++;
      }
    }
    return i;
}

/* ============================= mruby/c codes ============================= */

void c_uart_init(mrb_vm *vm, mrb_value *v, int argc) {
    memset(str, 0, sizeof(str));
    IPC9bits.U2IP = 1;
    IPC9bits.U2IS = 0;
    TRISBbits.TRISB8 = 1;
    U2RXRbits.U2RXR = 0x0004;   //RB8->UART2:U2RX;
    RPB9Rbits.RPB9R = 0x0002;   //RB9->UART2:U2TX;
    U2MODE = 0x8008;
    U2STA = 0x0;
    U2TXREG = 0x0;
    // BaudRate = 19200; Frequency = 40MHz; BRG 129;
    U2BRG = 40000000 / 16 / GET_INT_ARG(1) - 1;
    //U2BRG = 129;
    U2STAbits.UTXEN = 1;        //TX_enable
    U2STAbits.URXEN = 1;        //RX_enable

    //Enabling UART
    U2MODEbits.ON = 1;
}

static void c_uart_write(mrb_vm *vm, mrb_value *v, int argc) {
    char *mo = mrbc_string_cstr(&v[1]);
    int size = mrbc_string_size(&v[1]);
    int i = 0;
    while (U2STAbits.TRMT == 0);
    while(i<size){
        if(mo[i]==0){break;}
        while (U2STAbits.TRMT == 0);
        U2TXREG = mo[i];
        i++;
    }
}

static void c_uart_read(mrb_vm *vm, mrb_value *v, int argc) {
    int txt_size = GET_INT_ARG(1);
    char txt_a[txt_size];
    char txt_buff[50];
    mrb_value text;
    if(sp>=txt_size){
        memcpy(txt_a,str,txt_size);
        text = mrbc_string_new_cstr(vm, txt_a);
        sp -= txt_size;
        memcpy(txt_buff,&str[txt_size],sp);
        memcpy(str,txt_buff,50);
        SET_RETURN(text);
    }else{
        SET_NIL_RETURN();
    }
}

static void c_uart_gets(mrb_vm *vm, mrb_value *v, int argc) {
    mrb_value text;
    char txt_buff[50];
    int i;
    for(i=0;i<50;i++){
        if(str[i]=='\n'){
            i++;
            char txt_a[i];
            memcpy(txt_a,str,i);
            text = mrbc_string_new_cstr(vm, txt_a);
            sp -= i;
            memcpy(txt_buff,&str[i],sp);
            memcpy(str,txt_buff,50);
            break;
        }
    }
    SET_RETURN(text);
}

static void c_uart_clear_buffer(mrb_vm *vm, mrb_value *v, int argc) {
    memset(str, 0, sizeof(str));
}

void mrbc_init_class_uart(struct VM *vm){
    mrb_class *uart;
    uart = mrbc_define_class(0, "UART",	mrbc_class_object);
    mrbc_define_method(0, uart, "new", c_uart_init);
    mrbc_define_method(0, uart, "read", c_uart_read);
    mrbc_define_method(0, uart, "gets", c_uart_gets);
    mrbc_define_method(0, uart, "write", c_uart_write);
    mrbc_define_method(0, uart, "clear_buffer", c_uart_clear_buffer);
}