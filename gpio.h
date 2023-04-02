/* ************************************************************************** */
/** GPIO

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    gpio.h

  @Summary
    GPIO class processing

  @Description
    mruby/c function army
 */
/* ************************************************************************** */

#ifndef RBOARD_DIGITAL_H
#define RBOARD_DIGITAL_H


/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

/* C codes */
#define GPIO_IN			0x01
#define GPIO_OUT		0x02
#define GPIO_ANALOG		0x04
#define GPIO_HIGH_Z		0x08
#define GPIO_PULL_UP		0x10
#define GPIO_PULL_DOWN		0x20
#define GPIO_OPEN_DRAIN		0x40


int gpio_setmode( const PIN_HANDLE *pin, unsigned int mode );
void mrbc_init_class_gpio( void );


/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif
#endif /* RBOARD_DIGITAL_H */
