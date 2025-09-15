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

typedef void (*wakeup_handle_func)(uint8_t);
typedef void (*lock_status_handle_func)(void);
typedef void (*pvd_handle_func)(void);

ErrorStatus SetSysClockToMSI(void);
ErrorStatus SetSysClockToHSI(void);
ErrorStatus SetSysClockToHSE(void);
ErrorStatus SetSysClockToPLL(uint32_t freq, uint8_t src);

void board_init(void);
void pvd_init(pvd_handle_func h);
void wakeup_init(wakeup_handle_func h);
void lock_status_init(lock_status_handle_func h_on, lock_status_handle_func h_off);

#endif /* __BOARD_H__ */
