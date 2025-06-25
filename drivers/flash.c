#include "flash.h"

void flash_erase_page(uint32_t address)
{    
    /* Erase the page */
    FLASH_Unlock();
    if (FLASH_COMPL != FLASH_EraseOnePage(address)) {
        printf("Flash EraseOnePage Error at address 0x%08X. Please Deal With This Error Promptly\r\n", address);
    }
    FLASH_Lock();
    // printf("Flash Erase 0x%08X\r\n", address);
}

void flash_program_word(uint32_t address, uint32_t data)
{
    /* Program the word */
    FLASH_Unlock();
    if (FLASH_COMPL != FLASH_ProgramWord(address, data)) {
        printf("Flash ProgramWord Error at address 0x%08X. Please Deal With This Error Promptly\r\n", address);
    }
    FLASH_Lock();
    // printf("Flash Program 0x%08X 0x%08X\r\n", address, data);
}