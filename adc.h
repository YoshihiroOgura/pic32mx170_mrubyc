/* ************************************************************************** */
/** ADC

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    adc.h

  @Summary
    ADC processing

  @Description
    mruby/c function army
 */
/* ************************************************************************** */

#ifndef _ADC_H
#define _ADC_H

#include "mrubyc.h"

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

/* C codes */
void adc_init(void);

/* mruby/c codes */
void mrbc_init_class_adc(struct VM *vm);

/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _ADC_H */
