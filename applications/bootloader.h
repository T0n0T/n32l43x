#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include <stdint.h>
#include "n32l43x.h"

void app_start_final(uint32_t app_addr);

#endif // __BOOTLOADER_H