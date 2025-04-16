#ifndef __DRV_PM_H__
#define __DRV_PM_H__

#include <rthw.h>
#include <rtthread.h>
#include "board.h"
#include <n32l43x_lptim.h>

#ifdef __cplusplus
extern "C" {
#endif

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


#ifdef __cplusplus
}
#endif

#endif /* __DRV_PM_H__ */