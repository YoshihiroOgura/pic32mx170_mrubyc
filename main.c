#include "mrubyc.h"
#include <stdio.h>
#include <stdlib.h>
#include "delay.h"
#include "string.h"
#include <sys/attribs.h>
#include <math.h>
#include "timer.h"
// DEVCFG3
// USERID = No Setting
#pragma config PMDL1WAY = ON            // Peripheral Module Disable Configuration (Allow only one reconfiguration)
#pragma config IOL1WAY = ON             // Peripheral Pin Select Configuration (Allow only one reconfiguration)
#pragma config FUSBIDIO = OFF           // USB USID Selection (Controlled by Port Function)
#pragma config FVBUSONIO = OFF          // USB VBUS ON Selection (Controlled by Port Function)

// DEVCFG2
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_20         // PLL Multiplier (20x Multiplier)
#pragma config UPLLIDIV = DIV_2         // USB PLL Input Divider (2x Divider)
#pragma config UPLLEN = OFF             // USB PLL Enable (Disabled and Bypassed)
#pragma config FPLLODIV = DIV_4         // System PLL Output Clock Divider (PLL Divide by 2)

// DEVCFG1
#pragma config FNOSC = FRCPLL           // Oscillator Selection Bits (Fast RC Osc with PLL)
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disabled)
#pragma config IESO = ON                // Internal/External Switch Over (Enabled)
#pragma config POSCMOD = OFF            // Primary Oscillator Configuration (Primary osc disabled)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FPBDIV = DIV_2           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/2)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Clock Switch Disable, FSCM Disabled)
#pragma config WDTPS = PS1048576        // Watchdog Timer Postscaler (1:1048576)
#pragma config WINDIS = OFF             // Watchdog Timer Window Enable (Watchdog Timer is in Non-Window Mode)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (WDT Disabled (SWDTEN Bit Controls))
#pragma config FWDTWINSZ = WINSZ_25     // Watchdog Timer Window Size (Window Size is 25%)

// DEVCFG0
#pragma config JTAGEN = ON              // JTAG Enable (JTAG Port Enabled)
#pragma config ICESEL = ICS_PGx3        // ICE/ICD Comm Channel Select (Communicate on PGEC3/PGED3)
#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF                 // Code Protect (Protection Disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>

#define MEMORY_SIZE (1024*20)
static uint8_t memory_pool[MEMORY_SIZE];

int hal_write(int fd, const void *buf, int nbytes) {
    int i;
    while (U1STAbits.TRMT == 0);
    for (i = nbytes; i; --i) {
        while (U1STAbits.TRMT == 0);
        U1TXREG= *(char*) buf++;
    }
    return (nbytes);
}

int hal_flush(int fd) {
    return 0;
}

void pin_init(void){
    TRISAbits.TRISA4 = 1;
    TRISB |= 0x8c;
    CNPUB |= 0x8c;
}

static void c_pin_init(mrb_vm *vm, mrb_value *v, int argc) {
    pin_init();
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
    /* module init */
    i2c_init();
    adc_init();
    uart_init();
    timer_init();
    pin_init();
    
    /*Enable the interrupt*/
    IEC0bits.T1IE = 1;
    IEC0bits.T2IE = 1;
    IPC6bits.FCEIP = 1;
    IPC6bits.FCEIS = 0;
    IPC1bits.T1IP = 1;
    IPC1bits.T1IS = 0;
    IPC2bits.T2IP = 2;
    IPC2bits.T2IS = 0;
    INTCONbits.MVEC = 1;
    
    /* IDE code */
    add_code();
    uint8_t *addr = loadFlush();
    
    /* mruby/c */
    mrbc_init(memory_pool, MEMORY_SIZE);
    mrbc_define_method(0, mrbc_class_object, "pinInit", c_pin_init);
    mrbc_init_class_adc(0);
    mrbc_init_class_i2c(0);
    mrbc_init_class_uart(0);
    mrbc_init_class_digital(0);
    mrbc_init_class_timer(0);
    mrbc_init_class_pwm(0);
    mrbc_init_class_onboard(0);
    mrbc_create_task(addr, 0);
    T1CONbits.ON = 1;
    mrbc_run();
    return 1;
}
