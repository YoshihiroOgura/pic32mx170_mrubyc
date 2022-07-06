/* ************************************************************************** */
/** SPI

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    spi.h

  @Summary
    SPI processing

  @Description
    mruby/c function army
 */
/* ************************************************************************** */

#ifndef _SPI_H
#define _SPI_H

#include "mrubyc.h"

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif
    
/* C codes */
void spi_init(void);

/* mruby/c codes */
void mrbc_init_class_spi(struct VM *vm);

    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _I2C_H */
