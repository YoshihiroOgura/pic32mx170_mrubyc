#include <xc.h>
#include "mrubyc.h"
#include <stdio.h>
#include <stdlib.h>
#include "delay.h"
#include "string.h"
#include <sys/attribs.h>
#include <math.h>
#include "mrbc_firm.h"
#include "uart.h"

// DEVCFG3
// USERID = No Setting
#pragma config PMDL1WAY = ON            // Peripheral Module Disable Configuration (Allow only one reconfiguration)
#pragma config IOL1WAY = ON             // Peripheral Pin Select Configuration (Allow only one reconfiguration)

// DEVCFG2
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_20         // PLL Multiplier (20x Multiplier)
#pragma config FPLLODIV = DIV_4         // System PLL Output Clock Divider (PLL Divide by 2)

// DEVCFG1
#pragma config FNOSC = FRCPLL           // Oscillator Selection Bits (Fast RC Osc with PLL)
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disabled)
#pragma config IESO = ON                // Internal/External Switch Over (Enabled)
#pragma config POSCMOD = OFF            // Primary Oscillator Configuration (Primary osc disabled)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FPBDIV = DIV_2           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/2)
#pragma config FCKSM = CSECME           // Clock Switching and Monitor Selection (Clock Switch Enable, FSCM Enabled)
#pragma config WDTPS = PS32              // Watchdog Timer Postscaler (1:1)
#pragma config WINDIS = OFF             // Watchdog Timer Window Enable (Watchdog Timer is in Non-Window Mode)
#pragma config FWDTEN = OFF              // Watchdog Timer Disable (WDT Disable)

// DEVCFG0
#pragma config JTAGEN = OFF              // JTAG Enable (JTAG Port Enabled)
#pragma config ICESEL = ICS_PGx1        // ICE/ICD Comm Channel Select (Communicate on PGEC3/PGED3)
#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF                 // Code Protect (Protection Disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.


#define MEMORY_SIZE (1024*20)
uint8_t memory_pool[MEMORY_SIZE];
uint8_t t_count = 0;

int hal_write(int fd, const void *buf, int nbytes) {
  return uart_write(&uart1_handle, buf, nbytes );
}

int hal_flush(int fd) {
    return 0;
}

void pin_init(void){
    ANSELA = 0;
    ANSELB = 0;
    PORTA = 0;
    PORTB = 0;
    TRISAbits.TRISA4 = 1;
    TRISA &= 0xFC;
    TRISB &= 0xFC;
    TRISB |= 0x8c;
    CNPDA = 0x0;
    CNPUA = 0x0;
    CNPDB = 0x0;
    CNPUB |= 0x8c;
}

static void c_pin_init(mrb_vm *vm, mrb_value *v, int argc) {
    pin_init();
}

void timer_init() {
    TMR1 = TMR2 = TMR3 = 0x0;
    PR1 = PR2 = 10000;
    PR3 = 10;
    T1CON = T2CON = T3CON = 0x8000;
}

int check_timeout(void)
{
    int i;
    for( i = 0; i < 50; i++ ) {
        LATAbits.LATA0 = 1;
        __delay_ms( 30 );
        LATAbits.LATA0 = 0;
        __delay_ms( 30 );
        if( uart_can_read_line( &uart1_handle ) ) return 1;
    }
    return 0;
}

/* mruby/c writer */

void __ISR(_TIMER_1_VECTOR, IPL1AUTO) _T1Interrupt (  ){
    mrbc_tick();
    IFS0CLR= 1 << _IFS0_T1IF_POSITION;
}

void __ISR(_TIMER_2_VECTOR, IPL2AUTO) _T2Interrupt (  ){
    t_count++;
    IFS0CLR = 1 << _IFS0_T2IF_POSITION;
}

int main(void){
    //__XC_UART = 1;

    /* module init */
    i2c_init();
    adc_init();
    uart_init();
    timer_init();
    pin_init();

    /*Enable the interrupt*/
    IPC9bits.U2IP = 4;
    IPC9bits.U2IS = 3;
    IFS1bits.U2RXIF = 0;
    IFS1bits.U2TXIF = 0;
    IEC1bits.U2RXIE = 1;
    IEC1bits.U2TXIE = 0;
    IEC0bits.T1IE = 1;
    IEC0bits.T2IE = 1;
    IPC6bits.FCEIP = 1;
    IPC6bits.FCEIS = 0;
    IPC1bits.T1IP = 1;
    IPC1bits.T1IS = 0;
    IPC2bits.T2IP = 2;
    IPC2bits.T2IS = 0;
    INTCONbits.MVEC = 1;
    __builtin_enable_interrupts();

    //printf("\r\n\x1b(B\x1b)B\x1b[0m\x1b[2JRboard v*.*, mruby/c v2.1 start.\n");

    if (check_timeout()){
        /* IDE code */
        add_code();
    };

    /* mruby/c */
    mrbc_init(memory_pool, MEMORY_SIZE);
    mrbc_define_method(0, mrbc_class_object, "pinInit", c_pin_init);
    mrbc_init_class_adc(0);
    mrbc_init_class_i2c(0);
    mrbc_init_class_uart(0);
    mrbc_init_class_digital(0);
    mrbc_init_class_pwm(0);
    mrbc_init_class_onboard(0);
    mrbc_init_class_spi(0);
    int fl_addr = FLASH_SAVE_ADDR;
    uint8_t code_size_box[4];
    while(1){
        if(*((char *)fl_addr) != 'R'){
            break;
        }
        mrbc_create_task((void *)fl_addr, 0);
        memcpy(code_size_box, (void *)(fl_addr + 10), 4);
        int i = 0;
        int size = 0;
        for(i = 0;i < 4;i++){
            size += (code_size_box[i] << ((3-i)*8));
        }
	fl_addr = fl_addr + ROW_SIZE * ROW_COUNT( size );
    }
    T1CONbits.ON = 1;
    mrbc_run();
    return 1;
}
