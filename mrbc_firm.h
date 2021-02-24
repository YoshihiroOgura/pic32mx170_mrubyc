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

#define FLASH_SAVE_ADDR (0xBD032000)
#define PAGE_SIZE (1024)
#define MAX_SIZE (1024 * 40)
#define ROW_SIZE (PAGE_SIZE / sizeof(uint8_t) / 8)

#define ROW_COUNT(byte_size) ((byte_size) / ROW_SIZE + (byte_size % ROW_SIZE != 0))

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
