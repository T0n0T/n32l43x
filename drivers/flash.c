#include "flash.h"

void flash_erase_page(uint32_t address)
{
    /* Erase the page */
    FLASH_Unlock();
    assert_param(FLASH_COMPL == FLASH_EraseOnePage(address));
    FLASH_Lock();
}

void flash_program_word(uint32_t address, uint32_t data)
{
    /* Program the word */
    FLASH_Unlock();
    assert_param(FLASH_COMPL == FLASH_ProgramWord(address, data));
    FLASH_Lock();
}

void flash_erase_option(void)
{
    FLASH_Unlock();
    assert_param(FLASH_COMPL == FLASH_EraseOB());
    FLASH_Lock();
}

void flash_program_option(uint16_t data0_data1)
{
    uint32_t data0data1_tmp = (uint32_t)data0_data1 & 0xff | (uint32_t)(data0_data1 & 0xff00) << 8;
    FLASH_Unlock();
    assert_param(FLASH_COMPL == FLASH_ProgramOBData(0x1FFFF804, data0data1_tmp));
    FLASH_Lock();
}
