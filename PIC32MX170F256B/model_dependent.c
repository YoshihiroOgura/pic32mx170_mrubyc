/*
  MPU and board model related params.

  PIC32MX170F256B

  CPU clock 40MHz
   internal oscillator + PLL
  Peripheral clock 10MHz
*/

#include <xc.h>
#include "model_dependent.h"


// DEVCFG3
#pragma config PMDL1WAY = ON    // Peripheral Module Disable Configuration->Allow only one reconfiguration
#pragma config IOL1WAY = ON    // Peripheral Pin Select Configuration->Allow only one reconfiguration

// DEVCFG2
#pragma config FPLLIDIV = DIV_2    // PLL Input Divider->2x Divider
#pragma config FPLLMUL = MUL_20    // PLL Multiplier->20x Multiplier
#pragma config FPLLODIV = DIV_2    // System PLL Output Clock Divider->PLL Divide by 2

// DEVCFG1
#pragma config FNOSC = FRCPLL    // Oscillator Selection Bits->Fast RC Osc with PLL
#pragma config FSOSCEN = OFF    // Secondary Oscillator Enable->Disabled
#pragma config IESO = ON    // Internal/External Switch Over->Enabled
#pragma config POSCMOD = OFF    // Primary Oscillator Configuration->Primary osc disabled
#pragma config OSCIOFNC = OFF    // CLKO Output Signal Active on the OSCO Pin->Disabled
#pragma config FPBDIV = DIV_4    // Peripheral Clock Divisor->Pb_Clk is Sys_Clk/4
#pragma config FCKSM = CSDCMD    // Clock Switching and Monitor Selection->Clock Switch Disable, FSCM Disabled
#pragma config WDTPS = PS1048576    // Watchdog Timer Postscaler->1:1048576
#pragma config WINDIS = OFF    // Watchdog Timer Window Enable->Watchdog Timer is in Non-Window Mode
#pragma config FWDTEN = OFF    // Watchdog Timer Enable->WDT Disabled (SWDTEN Bit Controls)
#pragma config FWDTWINSZ = WINSZ_25    // Watchdog Timer Window Size->Window Size is 25%

// DEVCFG0
#pragma config DEBUG = OFF    // Background Debugger Enable->Debugger is Disabled
#pragma config JTAGEN = OFF    // JTAG Enable->JTAG Disabled
#pragma config ICESEL = ICS_PGx1    // ICE/ICD Comm Channel Select->Communicate on PGEC1/PGED1
#pragma config PWP = OFF    // Program Flash Write Protect->Disable
#pragma config BWP = OFF    // Boot Flash Write Protect bit->Protection Disabled
#pragma config CP = OFF    // Code Protect->Protection Disabled



/*! Remappable registers table.
*/
volatile uint32_t *TBL_RPxnR[] = { &RPA0R, &RPB0R, &RPC0R };



/*! Initializes the device to the default states configured.
*/
__attribute__((weak))
void system_init()
{
  /*
    Pin settings.

    方針: ハード的に用途が決定しているもの以外は、
    最も安全と思われる、全デジタル入力、内部プルダウンとしておく。

    RA  15  14  13  12  | 11  10  9   8   | 7   6   5   4   | 3   2   1   0
        --  --  --  --    --  --  --  --    --  --  --  Rx1           LED LED

    RB  15  14  13  12  | 11  10  9   8   | 7   6   5   4   | 3   2   1   0
        AN  AN                    Tx2 Rx2   SW          Tx1   SCL SDA
  */
  // Setting the Output Latch SFR(s)
  LATA = 0x0000;
  LATB = 0x0000;

  // Setting the GPIO Direction SFR(s)
  TRISA = 0x001C;
  TRISB = 0xFDE0;

  // Setting the Weak Pull Up and Weak Pull Down SFR(s)
  CNPDA = 0x000C;
  CNPDB = 0x3C60;

  CNPUA = 0x0000;
  CNPUB = 0x0080;

  // Setting the Open Drain SFR(s)
  ODCA = 0x0000;
  ODCB = 0x0000;

  // Setting the Analog/Digital Configuration SFR(s)
  ANSELA = 0x0000;
  ANSELB = 0xC000;

  /*
    Oscillator
  */
  // set this if you need.

  /*
    Interrupt
  */
  // Enable the multi vector
  INTCONbits.MVEC = 1;
  // Enable Global Interrupts
  __builtin_mtc0(12,0,(__builtin_mfc0(12,0) | 0x0001));
}


/*! onboard LED control.

  @param num	LED number. 1 origin.
  @param on_off	ON or OFF (True or False)
*/
__attribute__((weak))
void onboard_led( int num, int on_off )
{
  switch( num ) {
  case 1: LATAbits.LATA0 = !!on_off;	break;
  case 2: LATAbits.LATA1 = !!on_off;	break;
  case 3: LATBbits.LATB0 = !!on_off;	break;
  case 4: LATBbits.LATB1 = !!on_off;	break;
  }
}


/*! onboard SW control.

  @param num	SW number. 1 origin.
  @return	1 is OFF, 0 is ON.
*/
__attribute__((weak))
int onboard_sw( int num )
{
  return PORTBbits.RB7;
}
