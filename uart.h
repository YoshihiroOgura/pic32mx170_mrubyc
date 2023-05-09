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

#ifndef RBOARD_UART_H
#define RBOARD_UART_H

#include <stdint.h>
#include <string.h>
#include "pic32mx.h"


/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef UART_SIZE_RXFIFO
# define UART_SIZE_RXFIFO 128
#endif

//================================================================
/*!@brief
  UART Handle
*/
typedef struct UART_HANDLE {
  PIN_HANDLE txd_pin;
  PIN_HANDLE rxd_pin;

  uint8_t unit_num;		// 1..
  uint8_t rx_overflow;		// buffer overflow flag.
  uint8_t delimiter;

  volatile uint16_t rx_rd;	// index of rxfifo for read.
  volatile uint16_t rx_wr;	// index of rxfifo for write.
  volatile uint8_t rxfifo[UART_SIZE_RXFIFO]; // FIFO for received data.

} UART_HANDLE;

extern UART_HANDLE uart_handle_[];
#define UART_CONSOLE (&uart_handle_[0])

/*
  function prototypes.
*/
void uart_init(void);
void uart_enable(const UART_HANDLE *uh);
void uart_disable(const UART_HANDLE *uh);
int uart_setmode(const UART_HANDLE *uh, int baud, int parity, int stop_bits);
void uart_clear_rx_buffer(UART_HANDLE *uh);
int uart_read(UART_HANDLE *uh, void *buffer, int size);
int uart_write(UART_HANDLE *uh, const void *buffer, int size);
int uart_bytes_available(const UART_HANDLE *uh);
int uart_can_read_line(const UART_HANDLE *uh);
void mrbc_init_class_uart(void);


//================================================================
/*! check data can be read.

  @memberof UART_HANDLE
  @param  uh		Pointer of UART_HANDLE.
  @return int		result (bool)
*/
static inline int uart_is_readable( const UART_HANDLE *uh )
{
  return uh->rx_rd != uh->rx_wr;
}


//================================================================
/*! check Rx buffer overflow?

  @memberof UART_HANDLE
  @return int		result (bool)
*/
static inline int uart_is_rx_overflow( const UART_HANDLE *uh )
{
  return uh->rx_overflow;
}


//================================================================
/*! Transmit string.

  @memberof UART_HANDLE
  @param  uh		Pointer of UART_HANDLE.
  @param  s		Pointer of buffer.
  @return		Size of transmitted.
*/
static inline int uart_puts( UART_HANDLE *uh, const void *s )
{
  return uart_write( uh, s, strlen(s) );
}


/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif
#endif /* RBOARD_UART_H */
