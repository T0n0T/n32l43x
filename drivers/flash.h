#ifndef __FLASH_H__
#define __FLASH_H__
#include "board.h"

void flash_erase_page(uint32_t address);
void flash_program_word(uint32_t address, uint32_t data);
void flash_erase_option(void);
void flash_program_option(uint16_t data0_data1);

static inline uint16_t flash_option_get(void)
{
    return (uint16_t)(OBT->Data1_Data0 & 0xff) | (uint16_t)((OBT->Data1_Data0 >> 8) & 0xff00);
}

#endif /* __FLASH_H__ */
