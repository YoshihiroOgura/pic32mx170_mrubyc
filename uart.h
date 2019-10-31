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

/* C codes */
void uart_init(void);
void u_puts(char *mo, int size);
int u_read(char *addr);

/* mruby/c codes */
void mrbc_init_class_uart(struct VM *vm);

    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _UART_H */