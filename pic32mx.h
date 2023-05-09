/*
  PIC32MX Related functions.
*/

#ifndef PIC32MX_H
#define PIC32MX_H

#include <xc.h>
#include <stdint.h>
#include "model_dependent.h"

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

/*! Pin handle struct.

*/
typedef struct PIN_HANDLE {
  uint8_t port;		// A=1,B=2,..,G=7
  uint8_t num;		// 0..15
} PIN_HANDLE;


void __delay_us(uint32_t us);
void __delay_ms(uint32_t ms);
void system_register_lock(void);
void system_register_unlock(void);
void system_reset(void);

struct RObject;
int set_pin_handle( PIN_HANDLE *pin_handle, const struct RObject *val );


/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif


#endif /* PIC32MX_H */
