#ifndef __BOARD_H__
#define __BOARD_H__

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "n32l43x.h"

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

ErrorStatus SetSysClockToMSI(void);
ErrorStatus SetSysClockToHSI(void);
ErrorStatus SetSysClockToHSE(void);
ErrorStatus SetSysClockToPLL(uint32_t freq, uint8_t src);

typedef void (*wakeup_handle_func)(uint8_t);

void board_init(void);
void dump_clk(void);
void wakeup_pin_init(wakeup_handle_func h);

#endif /* __BOARD_H__ */
