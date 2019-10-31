/* ************************************************************************** */
/** timer

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    timer.c

  @Summary
    timer processing

  @Description
    mruby/c function army
 */
/* ************************************************************************** */

#ifndef _TIMER_H
#define _TIMER_H

#include "mrubyc.h"

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

/* C codes */
extern uint8_t t_count;
void timer_init(void);

/* mruby/c codes */
void mrbc_init_class_timer(struct VM *vm);

    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _TIMER_H */

