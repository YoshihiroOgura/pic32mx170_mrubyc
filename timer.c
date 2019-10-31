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

#include "timer.h"

/* ================================ C codes ================================ */

void timer_init() {
    TMR1 = TMR2 = TMR3 = 0x0;
    PR1 = PR2 = 10000;
    PR3 = 10;
    T1CON = T2CON = T3CON = 0x8000;
}

/* ============================= mruby/c codes ============================= */

static void timer_start(mrb_vm *vm, mrb_value *v, int argc) {
    T2CONbits.ON = 1;
}

static void timer_stop(mrb_vm *vm, mrb_value *v, int argc) {
    T2CONbits.ON = 0;
}

static void timer_set(mrb_vm *vm, mrb_value *v, int argc) {
    PR2 = GET_INT_ARG(1);
    T2CONbits.TCKPS = GET_INT_ARG(2);
}

static void timer_count_clear(mrb_vm *vm, mrb_value *v, int argc) {
    t_count = 0;
}

static void timer_count_get(mrb_vm *vm, mrb_value *v, int argc) {
    SET_INT_RETURN(t_count);
}

void mrbc_init_class_timer(struct VM *vm){
    mrb_class *timer;
    timer = mrbc_define_class(0, "Timer",	mrbc_class_object);
    mrbc_define_method(0, timer, "start", timer_start);
    mrbc_define_method(0, timer, "stop", timer_stop);
    mrbc_define_method(0, timer, "set", timer_set);
    mrbc_define_method(0, timer, "count_clear", timer_count_clear);
    mrbc_define_method(0, timer, "count_get", timer_count_get);
}
