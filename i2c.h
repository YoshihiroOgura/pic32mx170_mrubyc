/* ************************************************************************** */
/** I2C

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    i2c.h

  @Summary
    I2C processing

  @Description
    mruby/c function army
 */
/* ************************************************************************** */

#ifndef _I2C_H
#define _I2C_H

#include "mrubyc.h"

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

/* C codes */
void i2c_init(void);

/* mruby/c codes */
void mrbc_init_class_i2c(struct VM *vm);

/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _I2C_H */
