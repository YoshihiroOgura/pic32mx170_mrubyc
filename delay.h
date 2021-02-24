/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.h

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

#ifndef _DELAY_H    /* Guard against multiple inclusion */
#define _DELAY_H


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

/* This section lists the other files that are included in this file.
 */
/* TODO:  Include other files here if needed. */

#include <stdint.h>

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

void __delay_us( uint32_t us );
void __delay_ms( uint32_t ms );

static inline void delay( int n ) {
  __delay_ms( n );
}

#ifdef __cplusplus
}
#endif

#endif /* _DELAY_H */

/* *****************************************************************************
 End of File
 */
