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

#ifndef _UART_H
#define _UART_H

#include "mrubyc.h"

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
  uint8_t rx_overflow;		// buffer overflow flag.
  uint8_t delimiter;

  volatile uint16_t rx_rd;	// index of rxfifo for read.
  volatile uint16_t rx_wr;	// index of rxfifo for write.
  volatile uint8_t rxfifo[UART_SIZE_RXFIFO]; // FIFO for received data.

  void (*set_baudrate)();
  void (*clear_rx_buffer)();
  int (*write)();

} UART_HANDLE;

extern UART_HANDLE uart1_handle, uart2_handle;


//================================================================
/*! set baud-rate

  @memberof UART_HANDLE
  @param  uh		Pointer of UART_HANDLE.
  @param  baudrate	baudrate.
*/
static inline void uart_set_baudrate( const UART_HANDLE *uh, int baudrate )
{
  uh->set_baudrate( baudrate );
}


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
/*! Clear receive buffer.

  @memberof UART_HANDLE
  @param  uh		Pointer of UART_HANDLE.
*/
static inline void uart_clear_rx_buffer( UART_HANDLE *uh )
{
  uh->clear_rx_buffer(uh);
}


//================================================================
/*! Send out binary data.

  @memberof UART_HANDLE
  @param  uh            Pointer of UART_HANDLE.
  @param  buffer        Pointer of buffer.
  @param  size          Size of buffer.
  @return               Size of transmitted.
*/
static inline int uart_write( UART_HANDLE *uh, const void *buffer, int size )
{
  return uh->write( buffer, size );
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
  return uh->write( s, strlen(s) );
}




/* C codes */
void uart_init(void);
void u_puts(char *mo, int size);
int u_read(char *addr);

int uart_read(UART_HANDLE *uh, void *buffer, size_t size);
int uart_bytes_available(UART_HANDLE *uh);
int uart_can_read_line(UART_HANDLE *uh);


/* mruby/c codes */
void mrbc_init_class_uart(struct VM *vm);


/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _UART_H */
