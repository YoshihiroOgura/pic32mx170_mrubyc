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

#ifndef RBOARD_ADC_H
#define RBOARD_ADC_H


/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

/* C codes */
void adc_init(void);

/* mruby/c codes */
void mrbc_init_class_adc(void);

/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* RBOARD_ADC_H */
