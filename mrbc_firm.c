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
#include <stdint.h>

#include "pic32mx.h"
#include "uart.h"
#include "mrubyc.h"

#define VERSION_STRING   "mruby/c v3.3 RITE0300 MRBW1.2"

static const char RITE[4] = "RITE";
static const char WHITE_SPACE[] = " \t\r\n\f\v";
static uint8_t *irep_write_addr_ = (uint8_t*)FLASH_SAVE_ADDR;


/* wrap communication API macro.
*/
#define STRM_READ(buf, len)  uart_read(UART_HANDLE_CONSOLE, buf, len)
#define STRM_GETS(buf, size) uart_gets(UART_HANDLE_CONSOLE, buf, size)
#define STRM_PUTS(buf)       uart_puts(UART_HANDLE_CONSOLE, buf)
#define STRM_RESET()         uart_clear_rx_buffer(UART_HANDLE_CONSOLE)
#define SYSTEM_RESET()       do { \
  __builtin_disable_interrupts(); \
  system_reset(); \
} while(0)


/* function prototypes.
*/
static int cmd_help();
static int cmd_version();
static int cmd_reset();
static int cmd_execute();
static int cmd_clear();
static int cmd_write();
static int cmd_showprog();


/* command table.
*/
static struct COMMAND_T {
  const char *command;
  int (*function)();

} const TBL_COMMANDS[] = {
  {"help",	cmd_help },
  {"version",	cmd_version },
  {"reset",	cmd_reset },
  {"execute",	cmd_execute },
  {"clear",	cmd_clear },
  {"write",	cmd_write },
  {"showprog",	cmd_showprog },
};

static const int NUM_TBL_COMMANDS = sizeof(TBL_COMMANDS)/sizeof(struct COMMAND_T);


//================================================================
/*! command 'help'
*/
static int cmd_help(void)
{
  STRM_PUTS("+OK\r\nCommands:\r\n");

  for( int i = 0; i < NUM_TBL_COMMANDS; i++ ) {
    STRM_PUTS("  ");
    STRM_PUTS(TBL_COMMANDS[i].command);
    STRM_PUTS("\r\n");
  }
  STRM_PUTS("+DONE\r\n");
  return 0;
}


//================================================================
/*! command 'version'
*/
static int cmd_version(void)
{
  STRM_PUTS("+OK " VERSION_STRING "\r\n");
  return 0;
}


//================================================================
/*! command 'reset'
*/
static int cmd_reset(void)
{
  SYSTEM_RESET();
  return 0;
}


//================================================================
/*! command 'execute'
*/
static int cmd_execute(void)
{
  STRM_PUTS("+OK Execute mruby/c.\r\n");
  return 1;	// to execute VM.
}


//================================================================
/*! command 'clear'
*/
static int cmd_clear(void)
{
  irep_write_addr_ = (uint8_t*)FLASH_SAVE_ADDR;
  if( flash_erase_page( irep_write_addr_ ) == 0 ) {
    STRM_PUTS("+OK\r\n");
  } else {
    STRM_PUTS("-ERR\r\n");
  }
  return 0;
}


//================================================================
/*! command 'write'
*/
static int cmd_write( void *buffer, int buffer_size )
{
  char *token = strtok( NULL, WHITE_SPACE );
  if( token == NULL ) {
    STRM_PUTS("-ERR\r\n");
    return -1;
  }

  // check size
  int size = mrbc_atoi(token, 10);
  uint8_t *irep_write_end = irep_write_addr_ + size;
  if( (irep_write_end > (uint8_t *)FLASH_END_ADDR) || (size > buffer_size) ) {
    STRM_PUTS("-ERR IREP file size overflow.\r\n");
    return -1;
  }

  STRM_PUTS("+OK Write bytecode.\r\n");

  // get a bytecode.
  uint8_t *p = buffer;
  int n = size;
  while (n > 0) {
    int readed_size = STRM_READ( p, size );
    p += readed_size;
    n -= readed_size;
  }

  // check 'RITE' magick code.
  p = buffer;
  if( strncmp( (const char *)p, RITE, sizeof(RITE)) != 0 ) {
    STRM_PUTS("-ERR No RITE code received.\r\n");
    return -1;
  }

  // Write bytecode to FLASH.
  uint8_t *page_top = irep_write_addr_ - ((uintptr_t)irep_write_addr_ % FLASH_PAGE_SIZE);
  uint8_t *next_page_top = page_top + FLASH_PAGE_SIZE;
  uint8_t *prog_end_row = irep_write_addr_ + FLASH_ALIGN_ROW_SIZE(size);

  // erase required amount of PAGE
  if( irep_write_addr_ == page_top ) next_page_top = page_top;
  while( next_page_top < prog_end_row ) {
    if( (next_page_top + FLASH_PAGE_SIZE) > (uint8_t*)(FLASH_END_ADDR+1) ) {
      STRM_PUTS("-ERR total bytecode size overflow.\r\n");
      return -1;
    }

    if( flash_erase_page( next_page_top ) != 0 ) {
      STRM_PUTS("-ERR Flash erase error.\r\n");
      return -1;
    }
    next_page_top += FLASH_PAGE_SIZE;
  }

  // Write data to FLASH. segmentad by ROW size.
  while( irep_write_addr_ < prog_end_row ) {
    if( flash_write_row( irep_write_addr_, p ) != 0 ) {
      STRM_PUTS("-ERR Flash write error.\r\n");
      return -1;
    }
    irep_write_addr_ += FLASH_ROW_SIZE;
    p += FLASH_ROW_SIZE;
  }

  // Check if magic word "RITE" remains on the next rows.
  if( irep_write_addr_ == next_page_top &&
      memcmp( (const char *)next_page_top, RITE, sizeof(RITE)) == 0 ) {
    flash_erase_page( next_page_top );   // erase it.
  }

  STRM_PUTS("+DONE\r\n");

  return 0;
}


//================================================================
/*! command 'showprog'
*/
static int cmd_showprog(void)
{
  const uint8_t *addr = (const uint8_t *)FLASH_SAVE_ADDR;
  int n = 0;
  char buf[80];

  STRM_PUTS("idx size offset\r\n");
  while( strncmp( (const char *)addr, RITE, sizeof(RITE)) == 0 ) {
    uint32_t size = 0;
    for( int i = 0; i < 4; i++ ) {
      size = (size << 8) | addr[8 + i];
    }
    mrbc_snprintf(buf, sizeof(buf), " %d  %-4d %p\r\n", n++, size, addr);
    STRM_PUTS(buf);

    addr += FLASH_ALIGN_ROW_SIZE( size );
  }

  int total = (FLASH_END_ADDR - FLASH_SAVE_ADDR + 1);
  int used = addr - (uint8_t *)FLASH_SAVE_ADDR;
  int percent = 100 * used / total;
  mrbc_snprintf(buf, sizeof(buf), "total %d / %d (%d%%)\r\n", used, total, percent);
  STRM_PUTS(buf);
  STRM_PUTS("+DONE\r\n");

  return 0;
}


//================================================================
/*! receive bytecode mode
*/
int receive_bytecode( void *buffer, int buffer_size )
{
  char buf[50];

  STRM_PUTS("+OK mruby/c\r\n");

  while( 1 ) {
    // get the command string.
    if( STRM_GETS(buf, sizeof(buf)) < 0 ) {
      STRM_RESET();
      continue;
    }

    // split tokens.
    char *token = strtok( buf, WHITE_SPACE );
    if( token == NULL ) {
      STRM_PUTS("+OK mruby/c\r\n");
      continue;
    }

    // find command.
    int i;
    for( i = 0; i < NUM_TBL_COMMANDS; i++ ) {
      if( strcmp( token, TBL_COMMANDS[i].command ) == 0 ) break;
    }
    if( i == NUM_TBL_COMMANDS ) {
      STRM_PUTS("-ERR Illegal command. '");
      STRM_PUTS(token);
      STRM_PUTS("'\r\n");
      continue;
    }

    // execute command.
    if( (TBL_COMMANDS[i].function)(buffer, buffer_size) == 1 ) break;
  }

  return 0;
}


//================================================================
/*! pick up a task
*/
void * pickup_task( void *task )
{
  uint8_t *addr = (uint8_t *)FLASH_SAVE_ADDR;

  if( task ) {
    if( strncmp( task, RITE, sizeof(RITE)) != 0 ) return 0;

    addr = task;
    unsigned int size = 0;
    for( int i = 0; i < 4; i++ ) {
      size = (size << 8) | addr[8 + i];
    }

    addr += FLASH_ALIGN_ROW_SIZE( size );
  }

  if( strncmp( (const char *)addr, RITE, sizeof(RITE)) == 0 ) {
    return addr;
  }

  return 0;
}
