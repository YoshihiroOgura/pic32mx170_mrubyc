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


void __delay_us(uint32_t us);
void __delay_ms(uint32_t ms);
void system_register_lock(void);
void system_register_unlock(void);
void system_reset(void);
unsigned int NVMUnlock(unsigned int nvmop);
unsigned int flash_erase_page(void *address);
unsigned int flash_write_row(void *address, void *data);


/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif


#endif /* PIC32MX_H */
