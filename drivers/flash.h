#ifndef __FLASH_H__
#define __FLASH_H__
#include "board.h"

void flash_erase_page(uint32_t address);
void flash_program_word(uint32_t address, uint32_t data);

#endif /* __FLASH_H__ */
