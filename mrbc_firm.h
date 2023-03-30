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

#define FLASH_ALIGN_ROW_SIZE(byte_size) ((byte_size) + (((byte_size) % FLASH_ROW_SIZE) == 0 ? 0 : (FLASH_ROW_SIZE - ((byte_size) % FLASH_ROW_SIZE))))

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
