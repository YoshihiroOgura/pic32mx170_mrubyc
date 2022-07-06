#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/attribs.h>
#include <string.h>
//#include <math.h>

#include "mrubyc.h"
#include "adc.h"
#include "delay.h"
#include "digital.h"
#include "i2c.h"
#include "mrbc_firm.h"
#include "spi.h"
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
#pragma config WDTPS = PS8192           // Watchdog Timer Postscaler->1:8192
#pragma config WINDIS = OFF             // Watchdog Timer Window Enable (Watchdog Timer is in Non-Window Mode)
#pragma config FWDTEN = OFF             // Watchdog Timer Disable (WDT Disable)

// DEVCFG0
#pragma config JTAGEN = OFF              // JTAG Enable (JTAG Port Enabled)
#pragma config ICESEL = ICS_PGx1        // ICE/ICD Comm Channel Select (Communicate on PGEC3/PGED3)
#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF                 // Code Protect (Protection Disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.


#define MEMORY_SIZE (1024*40)
uint8_t memory_pool[MEMORY_SIZE];
uint8_t t_count = 0;

//================================================================
/*! system register lock / unlock
*/
void system_register_lock(void)
{
  SYSKEY = 0x0;
}

void system_register_unlock(void)
{
  unsigned int status;

  // Suspend or Disable all Interrupts
  asm volatile ("di %0" : "=r" (status));

  // starting critical sequence
  SYSKEY = 0x33333333; // write invalid key to force lock
  SYSKEY = 0xAA996655; // write key1 to SYSKEY
  SYSKEY = 0x556699AA; // write key2 to SYSKEY

  // Restore Interrupts
  if (status & 0x00000001) {
    asm volatile ("ei");
  } else {
    asm volatile ("di");
  }
}


//================================================================
/*! software reset	DS60001118H
*/
void system_reset(void)
{
  system_register_unlock();

  RSWRSTSET = 1;

  // read RSWRST register to trigger reset
  uint32_t dummy = RSWRST;
  (void)dummy;

  while(1)
    ;
}

int hal_write(int fd, const void *buf, int nbytes) {
  return uart_write(&uart1_handle, buf, nbytes );
}

int hal_flush(int fd) {
    return 0;
}

void hal_abort( const char *s )
{
  if( s ) {
    hal_write( 0, s, strlen(s) );
  }
  __delay_ms(5000);
  system_reset();
}

void _mon_putc( char c )
{
  uart_write(&uart1_handle, &c, 1);
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



int main(void)
{
    /* module init */
    pin_init();
    i2c_init();
    adc_init();
    uart_init();
    timer_init();

    /*Enable the interrupt*/
#if 0
    IPC9bits.U2IP = 4;
    IPC9bits.U2IS = 3;
    IFS1bits.U2RXIF = 0;
    IFS1bits.U2TXIF = 0;
    IEC1bits.U2RXIE = 1;
    IEC1bits.U2TXIE = 0;
#endif
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

    printf("\r\n\x1b(B\x1b)B\x1b[0m\x1b[2JRboard v*.*, mruby/c v3.1 start.\n");

    if (check_timeout()){
        /* IDE code */
        add_code();
	memset( memory_pool, 0, sizeof(memory_pool));
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

    const uint8_t *fl_addr = (uint8_t*)FLASH_SAVE_ADDR;
    static const char RITE[4] = "RITE";
    while( strncmp( fl_addr, RITE, sizeof(RITE)) == 0 ) {
      mrbc_create_task(fl_addr, 0);

      // get a next irep.
      uint32_t size = 0;
      int i;
      for( i = 0; i < 4; i++ ) {
	size = (size << 8) | fl_addr[8 + i];
      }
      fl_addr += ALIGN_ROW_SIZE( size );
    }

    T1CONbits.ON = 1;
    mrbc_run();
    return 1;
}
