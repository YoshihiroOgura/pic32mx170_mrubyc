/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.c

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */

#include "uart.h"

void uart_init(void){
   ANSELA = 0x0000;
   IPC8bits.U1IP = 1;
   IPC8bits.U1IS = 0;
   TRISAbits.TRISA4 = 1;
   U1RXRbits.U1RXR = 0x0002;   //RA4->UART1:U1RX;
   RPB4Rbits.RPB4R = 0x0001;   //RB4->UART1:U1TX;
   U1MODE = 0x8008;
   // UTXISEL TX_ONE_CHAR; UTXINV disabled; ADDR 0; URXEN disabled; OERR disabled; ADM_EN disabled; URXISEL RX_ONE_CHAR; UTXBRK disabled; UTXEN disabled; ADDEN disabled; 
   U1STA = 0x0;
   // U1TXREG 0; 
   U1TXREG = 0x0;
   // BaudRate = 19200; Frequency = 40kHz; BRG 129; 
   U1BRG = 129;
   U1STAbits.UTXEN = 1;        //10_TX_enable
   U1STAbits.URXEN = 1;        //12_RX_enable

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
      if(PORTBbits.RB7 == 0){
          break;
      }
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

static void c_uart_puts(mrb_vm *vm, mrb_value *v, int argc) {
    char *mo = mrbc_string_cstr(&v[1]);
    int size = mrbc_string_size(&v[1]);
    int i = 0;
    while (U2STAbits.TRMT == 0);
    while(i<size){
        if(mo[i]==0){break;}
        while (U2STAbits.TRMT == 0);
        U2TXREG= mo[i];
        i++;
    }
}

static void c_uart_gets(mrb_vm *vm, mrb_value *v, int argc) {
    char *addr;
    int i = 0;
    mrb_value text;
    while(1){
        IFS1bits.U1RXIF = 0;
        while(!U1STAbits.URXDA);
        addr[i] = U1RXREG;
        if(addr[i] == 0x0a){
            break;
        }
        i++;
    }
    text = mrbc_string_new_cstr(vm, addr);
    SET_RETURN(text);
}

void mrbc_init_class_uart(struct VM *vm)
{
    mrb_class *uart;
    uart = mrbc_define_class(0, "UART",	mrbc_class_object);
    mrbc_define_method(0, uart, "gets", c_uart_gets);
    mrbc_define_method(0, uart, "puts", c_uart_puts);
}