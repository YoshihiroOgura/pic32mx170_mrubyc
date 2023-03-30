#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include "pic32mx.h"
#include "mrubyc.h"


/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct PIN_HANDLE {
  uint8_t port;		// A=1,B=2,..,G=7
  uint8_t num;		// 0..15
} PIN_HANDLE;


void __delay_us(uint32_t us);
void __delay_ms(uint32_t ms);
void system_register_lock(void);
void system_register_unlock(void);
void system_reset(void);
int set_pin_handle( PIN_HANDLE *pin_handle, const mrbc_value *val );


/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif


#endif /* COMMON_H */
