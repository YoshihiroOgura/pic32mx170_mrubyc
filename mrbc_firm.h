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

#define FLASH_SAVE_ADDR 0xBD032000
#define FLASH_END_ADDR  0xBD03EFFF
#define PAGE_SIZE 1024
#define ROW_SIZE (PAGE_SIZE / 8)

#define ALIGN_ROW_SIZE(byte_size) ((byte_size) + (((byte_size) % ROW_SIZE) == 0 ? 0 : (ROW_SIZE - ((byte_size) % ROW_SIZE))))

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

void add_code(void);

/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _MRBC_FIRM_H */
