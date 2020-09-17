/* ************************************************************************** */
/** DIGITAL

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    digital.h

  @Summary
    Digital & PWM processing

  @Description
    mruby/c function army
 */
/* ************************************************************************** */

#ifndef _DIGITAL_H
#define _DIGITAL_H

#include "mrubyc.h"

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

/* C codes */
typedef struct GPIO_HANDLE {
  int pin_num;
} GPIO_HANDLE;

/* mruby/c codes */
void mrbc_init_class_onboard(struct VM *vm);
void mrbc_init_class_digital(struct VM *vm);
void mrbc_init_class_pwm(struct VM *vm);

    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _DIGITAL_H */
