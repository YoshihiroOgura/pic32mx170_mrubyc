/* ************************************************************************** */
/** mrbc firm

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    mrbc_firm.c

  @Summary
    mruby/c IDE support

  @Description
    mruby/c IDE support & Flash writing
 */
/* ************************************************************************** */

#include <xc.h>
#include <stdio.h>
#include <stdint.h>
#include "mrbc_firm.h"
#include "uart.h"

#define MRUBYC_VERSION_STRING   "mruby/c v2.1 PIC32MX"

static const char WHITE_SPACE[] = " \t\r\n\f\v";
static uint8_t *p_irep_write = (uint8_t*)FLASH_SAVE_ADDR;
static uint8_t flag_clear = 0;

static int cmd_help(void);
static int cmd_version(void);
static int cmd_reset(void);
static int cmd_execute(void);
static int cmd_clear(void);
static int cmd_write(void);
static int cmd_showprog(void);

//================================================================
/*! command table.
*/
struct monitor_commands {
  const char *command;
  int (*function)(void);

} monitor_commands[] = {
  {"help",	cmd_help },
  {"version",	cmd_version },
  {"reset",	cmd_reset },
  {"execute",	cmd_execute },
  {"clear",	cmd_clear },
  {"write",	cmd_write },
  {"showprog",	cmd_showprog },
};


//================================================================
/*! NVM unlock and execute nvm operation.

  @param nvmop	NVM Operation bits. (see: DS60001121G, Sect 5.2.1)
  @return	not zero if errors.
*/
static unsigned int NVMUnlock(unsigned int nvmop)
{
  unsigned int status;

  // Suspend or Disable all Interrupts
  asm volatile ("di %0" : "=r" (status));

  // clearing error bits before performing an NVM operation
  NVMCONCLR = 0x0f;

  // Enable Flash Write/Erase Operations and Select
  // Flash operation to perform
  NVMCON = (0x4000 | nvmop);

  // Write Keys
  NVMKEY = 0xAA996655;
  NVMKEY = 0x556699AA;

  // Start the operation using the Set Register
  NVMCONSET = 0x8000;

  // Wait for operation to complete
  while (NVMCON & 0x8000);

  // Restore Interrupts
  if (status & 0x00000001) {
    asm volatile ("ei");
  } else {
    asm volatile ("di");
  }

  // Disable NVM write enable
  NVMCONCLR = 0x4000;

  // Return WRERR and LVDERR Error Status Bits
  return (NVMCON & 0x3000);
}


//================================================================
/*! Erase one *PAGE* of FLASH ROM

  @param  address	flash rom address
  @return		not zero if errors.
*/
static unsigned int flash_erase_page(void *address)
{
  NVMADDR = ((uintptr_t)address & 0x1FFFFFFF);
  return NVMUnlock( 0x4 );	// 0x4 = Page Erase Operation
}


//================================================================
/*! Write one *ROW* of FLASH ROM

  @param  address	flash rom address
  @return		not zero if errors.
*/
static unsigned int flash_write_row(void *address, void *data)
{
  NVMADDR = ((uintptr_t)address & 0x1FFFFFFF);
  NVMSRCADDR = ((uintptr_t)data & 0x1FFFFFFF);
  return NVMUnlock( 0x3 );	// 0x3 = Row Program Operation
}


//================================================================
/*! put string sub-routine.

  @param  s		pointer to string.
*/
static void u_puts( const void *s )
{
  uart_puts(&uart1_handle, s);
  uart_puts(&uart1_handle, "\r\n");
}


//================================================================
/*! command 'help'
*/
static int cmd_help(void)
{
  u_puts("+OK\r\nCommands:");

  int i;
  for( i = 0; i < sizeof(monitor_commands)/sizeof(struct monitor_commands); i++ ) {
    uart_puts(&uart1_handle, "  ");
    u_puts(monitor_commands[i].command);
  }
  u_puts("+DONE");
  return 0;
}


//================================================================
/*! command 'version'
*/
static int cmd_version(void)
{
  u_puts("+OK " MRUBYC_VERSION_STRING);
  return 0;
}


//================================================================
/*! command 'reset'
*/
static int cmd_reset(void)
{
  __builtin_disable_interrupts();

  // starting critical sequence
  SYSKEY = 0x33333333; //write invalid key to force lock
  SYSKEY = 0xAA996655; //write key1 to SYSKEY
  SYSKEY = 0x556699AA; //write key2 to SYSKEY

  RSWRSTbits.SWRST = 1;
  uint32_t dummy = RSWRST;
  (void)dummy;

  while(1)
    ;

  return 0;
}


//================================================================
/*! command 'execute'
*/
static int cmd_execute(void)
{
  u_puts("+OK Execute mruby/c.");
  return 1;	// to execute VM.
}


//================================================================
/*! command 'clear'
*/
static int cmd_clear(void)
{
  p_irep_write = (uint8_t*)FLASH_SAVE_ADDR;
  flag_clear = 1;
  if( flash_erase_page( (uint8_t*)FLASH_SAVE_ADDR ) == 0 ) {
    u_puts("+OK");
  } else {
    u_puts("+ERR");
  }
  return 0;
}


//================================================================
/*! command 'write'
*/
static int cmd_write(void)
{
  if( !flag_clear ) {
    flag_clear = 1;
    flash_erase_page( (uint8_t*)FLASH_SAVE_ADDR );
  }

  char *token = strtok( NULL, WHITE_SPACE );
  if( token == NULL ) {
    u_puts("-ERR");
    return 0;
  }

  int size = atoi(token);
  // TODO: size check.

  u_puts("+OK Write bytecode.");

  extern uint8_t memory_pool[];
  uint8_t *p = memory_pool;
  int n = size;
  while (n > 0) {
    int readed_size = uart_read( &uart1_handle, p, size );
    p += readed_size;
    n -= readed_size;
  }

  // erase required amount of PAGE
  uint8_t *page_top = p_irep_write - ((uintptr_t)p_irep_write % PAGE_SIZE);
  uint8_t *next_page_top = page_top + PAGE_SIZE;
  uint8_t *prog_end_addr = p_irep_write + ALIGN_ROW_SIZE(size);

  if( prog_end_addr > (uint8_t*)FLASH_END_ADDR ) {
    u_puts("-ERR total bytecode size overflow.");
    goto DONE;
  }

  while( prog_end_addr >= next_page_top ) {
    if( flash_erase_page( next_page_top ) != 0 ) {
      u_puts("-ERR Flash erase error.");
      goto DONE;
    }
    next_page_top += PAGE_SIZE;
  }

  // Write data to FLASH. segmentad by ROW size.
  p = memory_pool;
  while( p_irep_write < prog_end_addr ) {
    if( flash_write_row( p_irep_write, p ) != 0 ) {
      u_puts("-ERR Flash write error.");
      goto DONE;
    }
    p_irep_write += ROW_SIZE;
    p += ROW_SIZE;
  }

  u_puts("+DONE");

 DONE:
  return 0;
}


//================================================================
/*! command 'showprog'
*/
static int cmd_showprog(void)
{
  static const char RITE[4] = "RITE";
  const uint8_t *fl_addr = (const uint8_t *)FLASH_SAVE_ADDR;
  uint32_t used_size = 0;
  int n = 0;
  char buf[32];

  u_puts("idx   size");
  while( strncmp( fl_addr, RITE, sizeof(RITE)) == 0 ) {
    uint32_t size = 0;
    int i;
    for( i = 0; i < 4; i++ ) {
      size = (size << 8) | fl_addr[8 + i];
    }
    size = ALIGN_ROW_SIZE( size );
    used_size += size;
    fl_addr += size;

    sprintf(buf, " %d  %d", n++, size );
    u_puts( buf );
  }

  int total_size = (FLASH_END_ADDR - FLASH_SAVE_ADDR + 1);
  int percent = 100 * used_size / total_size;
  sprintf(buf, "total %d / %d (%d%%)", used_size, total_size, percent);
  u_puts( buf );
  u_puts("+DONE");

  return 0;
}


//================================================================
/*! mruby/c IDE program creation process
*/
void add_code(void)
{
  char buf[50];

  while( 1 ) {
    // get the command string.
    int len = uart_can_read_line(&uart1_handle);
    if( !len ) continue;

    if( len >= sizeof(buf) ) {
      uart_clear_rx_buffer(&uart1_handle);
      continue;
    }
    uart_read(&uart1_handle, buf, len);
    buf[len] = 0;

    // split tokens.
    char *token = strtok( buf, WHITE_SPACE );
    if( token == NULL ) {
      u_puts("+OK mruby/c");
      continue;
    }

    // execute command.
    int i;
    for( i = 0; i < sizeof(monitor_commands)/sizeof(struct monitor_commands); i++ ) {
      if( strcmp( token, monitor_commands[i].command ) == 0 ) {
	int ret = (monitor_commands[i].function)();
	switch( ret ) {
	case 0:
	  goto DONE;
	case 1:
	  return;
	}
      }
    }

    u_puts("-ERR Illegal command.");

  DONE:
    ;
  }
}
