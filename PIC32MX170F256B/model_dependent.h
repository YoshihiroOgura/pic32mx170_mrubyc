/*
  MPU and board model related params.

  PIC32MX170F256B
    * MIPS32® M4K® core with MIPS16e® mode
    * Up to 50 MHz/83 DMIPS
    * 256KB Flash memory (plus an additional 3 KB of Boot Flash)
    * 64K SRAM memory

  Current settings
    * CPU clock 40MHz
      internal oscillator + PLL
    * Peripheral clock 10MHz
*/

#if !defined(__32MX170F256B__) && !defined(__PIC32MX170F256B__)
# error "Change the project property, xc32-gcc Include directories to the MPU you want to use."
#endif

#ifndef MODEL_DEPENDENT_H
#define MODEL_DEPENDENT_H

#include <xc.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! get offset of port address.

    以下のレジスタが、番号ごとに一定番地離れている事に依存しています。
      * GPIO 関連のレジスタが、ポートごとに 0x100番地
      * OC(PWM) 関連のレジスタが、0x200番地
    そうでないプロセッサに対応が必要になった場合は、戦略の変更が必要です。
*/
// Pin settings
#define ANSELxSET(x)	*(&ANSELASET + (0x100 / sizeof(uint32_t)) * ((x)-1))
#define ANSELxCLR(x)	*(&ANSELACLR + (0x100 / sizeof(uint32_t)) * ((x)-1))
#define TRISxSET(x)	*(&TRISASET  + (0x100 / sizeof(uint32_t)) * ((x)-1))
#define TRISxCLR(x)	*(&TRISACLR  + (0x100 / sizeof(uint32_t)) * ((x)-1))
#define PORTx(x)	*(&PORTA     + (0x100 / sizeof(uint32_t)) * ((x)-1))
#define LATxSET(x)	*(&LATASET   + (0x100 / sizeof(uint32_t)) * ((x)-1))
#define LATxCLR(x)	*(&LATACLR   + (0x100 / sizeof(uint32_t)) * ((x)-1))
#define ODCxSET(x)	*(&ODCASET   + (0x100 / sizeof(uint32_t)) * ((x)-1))
#define ODCxCLR(x)	*(&ODCACLR   + (0x100 / sizeof(uint32_t)) * ((x)-1))
#define CNPUxSET(x)	*(&CNPUASET  + (0x100 / sizeof(uint32_t)) * ((x)-1))
#define CNPUxCLR(x)	*(&CNPUACLR  + (0x100 / sizeof(uint32_t)) * ((x)-1))
#define CNPDxSET(x)	*(&CNPDASET  + (0x100 / sizeof(uint32_t)) * ((x)-1))
#define CNPDxCLR(x)	*(&CNPDACLR  + (0x100 / sizeof(uint32_t)) * ((x)-1))

// Output comparator
#define NUM_PWM_OC_UNIT 5
#define OCxCON(x)	*(&OC1CON    + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define OCxR(x)		*(&OC1R      + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define OCxRS(x)	*(&OC1RS     + (0x200 / sizeof(uint32_t)) * ((x)-1))

// SPI
#define NUM_SPI_UNIT 2
#define SPIxCON(x)	*(&SPI1CON   + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define SPIxCONSET(x)	*(&SPI1CONSET+ (0x200 / sizeof(uint32_t)) * ((x)-1))
#define SPIxCONCLR(x)	*(&SPI1CONCLR+ (0x200 / sizeof(uint32_t)) * ((x)-1))
#define SPIxSTAT(x)	*(&SPI1STAT  + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define SPIxBUF(x)	*(&SPI1BUF   + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define SPIxBRG(x)	*(&SPI1BRG   + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define SPIxCON2(x)	*(&SPI1CON2  + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define SDIxR(x)	*(&SDI1R     + (0x00c / sizeof(uint32_t)) * ((x)-1))

// UART
#define NUM_UART_UNIT 2
#define UxMODE(x)	*(&U1MODE    + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define UxMODESET(x)	*(&U1MODESET + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define UxMODECLR(x)	*(&U1MODECLR + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define UxSTA(x)	*(&U1STA     + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define UxSTASET(x)	*(&U1STASET  + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define UxSTACLR(x)	*(&U1STACLR  + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define UxTXREG(x)	*(&U1TXREG   + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define UxRXREG(x)	*(&U1RXREG   + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define UxBRG(x)	*(&U1BRG     + (0x200 / sizeof(uint32_t)) * ((x)-1))
#define UxRXR(x)	*(&U1RXR     + (0x008 / sizeof(uint32_t)) * ((x)-1))
#define IPC_U1IPIS(ip,is)	(IPC8bits.U1IP = (ip), IPC8bits.U1IS = (is))
#define IPC_U2IPIS(ip,is)	(IPC9bits.U2IP = (ip), IPC9bits.U2IS = (is))

// Output pin selection.
#define RPxnR(x,n)	(TBL_RPxnR[(x)-1][n])
extern volatile uint32_t *TBL_RPxnR[];

/*! default pin assign.
*/
// UART
#define UART1_TXD_PIN	2,4     // B4
#define UART1_RXD_PIN	1,4     // A4
#define UART2_TXD_PIN	2,9	// B9
#define UART2_RXD_PIN	2,8	// B8
#define UART_CONSOLE	1

// flash memory address and parameter.
#define FLASH_SAVE_ADDR 0xBD037000
#define FLASH_END_ADDR  0xBD03EFFF
#define FLASH_PAGE_SIZE 1024
#define FLASH_ROW_SIZE (FLASH_PAGE_SIZE / 8)
#define FLASH_ALIGN_ROW_SIZE(bytes) \
  ((bytes) + ((FLASH_ROW_SIZE - (bytes)) & (FLASH_ROW_SIZE-1)))

// System clock.
#if !defined(_XTAL_FREQ)
#define _XTAL_FREQ  40000000UL
#endif

#if !defined(PBCLK)
#define PBCLK (_XTAL_FREQ / 4)
#endif


void system_init(void);
void onboard_led(int num, int on_off);
int onboard_sw(int num);


#ifdef __cplusplus
}
#endif
#endif  /* MODEL_DEPENDENT_H */
