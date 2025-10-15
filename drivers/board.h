#ifndef __BOARD_H__
#define __BOARD_H__

#include <string.h>
#include <stdio.h>
#include <stdint.h>
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

#define WAKE_SRC_KEY  (1 << 0)
#define WAKE_SRC_RFID (1 << 1)

typedef void (*wakeup_handle_func)(uint8_t, uint8_t);
typedef void (*pwr_handle_func)(void);
typedef void (*pvd_handle_func)(void);

ErrorStatus SetSysClockToMSI(void);
ErrorStatus SetSysClockToHSI(void);
ErrorStatus SetSysClockToHSE(void);
ErrorStatus SetSysClockToPLL(uint32_t freq, uint8_t src);

void board_init(void);
void pvd_init(pvd_handle_func h);
void wakeup_init(wakeup_handle_func h);

#endif /* __BOARD_H__ */
