/* ************************************************************************** */
/** mrbc firm

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    mrbc_firm.h

  @Summary
    mruby/c IDE support

  @Description
    mruby/c IDE support & Flash writing
 */
/* ************************************************************************** */

#ifndef _MRBC_FIRM_H    /* Guard against multiple inclusion */
#define _MRBC_FIRM_H

#include <xc.h>
#include <math.h>
#include<string.h>

#define FLASH_SAVE_ADDR0 (0xBD030000)
#define FLASH_SAVE_ADDR1 (0xBD035500)
#define FLASH_SAVE_ADDR2 (0xBD03AA00)

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif
    
uint8_t *loadFlush();
void add_code();

//void add_code(void);

    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _MRBC_FIRM_H */