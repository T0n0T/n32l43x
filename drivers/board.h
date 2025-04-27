#ifndef __BOARD_H__
#define __BOARD_H__

#include <stddef.h>
#include "n32l40x.h"
typedef enum {
    SYSCLK_PLLSRC_HSI,
    SYSCLK_PLLSRC_HSIDIV2,
    SYSCLK_PLLSRC_HSI_PLLDIV2,
    SYSCLK_PLLSRC_HSIDIV2_PLLDIV2,
    SYSCLK_PLLSRC_HSE,
    SYSCLK_PLLSRC_HSEDIV2,
    SYSCLK_PLLSRC_HSE_PLLDIV2,
    SYSCLK_PLLSRC_HSEDIV2_PLLDIV2,
} SYSCLK_PLL_TYPE;

void board_init(void);
void set_sysclock_to_pll(uint32_t freq, SYSCLK_PLL_TYPE src);

#endif /* __BOARD_H__ */
