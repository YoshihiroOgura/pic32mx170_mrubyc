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

typedef struct PWM_HANDLE {
  int pin_num;
} PWM_HANDLE;

int out_compare[21] = {1,2,4,3,4,3,2,4,1,1,2,4,1,2,3,3,2,0,4,3,1};

uint32_t* pwm_a=(&RPA0R);
uint32_t* pwm_b=(&RPB0R);

/* mruby/c codes */
void mrbc_init_class_onboard(struct VM *vm);
void mrbc_init_class_digital(struct VM *vm);
void mrbc_init_class_pwm(struct VM *vm);

    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _DIGITAL_H */
