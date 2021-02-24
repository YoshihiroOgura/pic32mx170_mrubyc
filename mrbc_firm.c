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
#include "mrbc_firm.h"


/*! NVM unlock and execute nvm operation.

  @param nvmop	NVM Operation bits. (see: DS60001121G, Sect 5.2.1)
  @return	not zero if errors.
*/
unsigned int NVMUnlock(unsigned int nvmop)
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


void flash_del(void* address)
{
  NVMADDR = ((unsigned int) address & 0x1FFFFFFF);
  NVMUnlock( 0x4 );	// 0x4 = Page Erase Operation
}

void flash_write(void* address, void* data)
{
  NVMADDR = ((unsigned int) address & 0x1FFFFFFF);
  NVMSRCADDR = ((unsigned int) data & 0x1FFFFFFF);
  NVMUnlock( 0x3 );	// 0x3 = Row Program Operation
}

static void saveFlush(const uint8_t* writeData, uint16_t size) {
    static int fl_addr = FLASH_SAVE_ADDR;
    static int sum_pageCount = 0;
    static int remaining_page_size = 0;

    int rowCount = (size % ROW_SIZE == 0) ? size / ROW_SIZE : size / ROW_SIZE + 1;
    int pageCount = (rowCount+remaining_page_size)/8;
    if(rowCount%8){
        remaining_page_size = rowCount%8;
        pageCount++;
    }

    // Initialize the required amount of ROM
    int i = 0;
    for(i = sum_pageCount;i < pageCount;i++) {
        flash_del((void *)(FLASH_SAVE_ADDR + i * PAGE_SIZE));
    }

    // Write data to ROM
    sum_pageCount += pageCount;
    for(i = 0;i < rowCount; i++) {
        flash_write((void *)(fl_addr + i * ROW_SIZE), (void *)&writeData[i * ROW_SIZE]);
    }
    fl_addr = fl_addr + ROW_SIZE*rowCount;
}

/*
  mruby / cIDE program creation process
*/
void add_code(){
    char txt[50];
    char *txt_addr;
    int txt_len;
    txt_addr = txt;
    // [crlf] read waite
    txt_len = u_read(txt_addr);
    if(!txt_len){
        return;
    }
    u_puts("+OK mruby/c\r\n",0);
    memset(txt, 0, sizeof(txt));
    while(1){
        txt_len = u_read(txt_addr);
        if(strncmp(txt_addr,"version",7)==0){
            u_puts("+OK mruby/c PSoC_5LP v1.00 (2018/09/04)\r\n",0);
            memset(txt, 0, sizeof(txt));
        }else if(strncmp(txt_addr,"execute",7)==0){
            u_puts("+OK Execute mruby/c.\r\n",0);
            memset(txt, 0, sizeof(txt));
            return;
        }else if(strncmp(txt_addr,"write",5)==0){
            // bytecode length receive
            u_puts("+OK Write bytecode\r\n",0);
            int size = 0;
            txt_len -= 2;
            int j = 0;
            while(txt_len>5){
                size += (txt[txt_len] - 0x30) * pow(10,j);
                j++;
                txt_len--;
            }

            unsigned int return_size = size;
            uint8_t data[size];

            // mruby/c code write
            int i = 0;
            while (size > 0) {
                IFS1bits.U1RXIF = 0;
                while(!U1STAbits.URXDA);
                data[i] = U1RXREG;
                size--;
                i++;
            }
            saveFlush(data, sizeof(data));
            u_puts("+DONE\r\n",0);
            memset(txt, 0, sizeof(txt));
        }
    }
}
