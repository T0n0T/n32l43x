#ifndef __DRV_LPTIMER_H__
#define __DRV_LPTIMER_H__

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include <n32l40x_lptim.h>

#ifdef __cplusplus
extern "C" {
#endif

struct n32_lptimer {
    rt_hwtimer_t    time_device;  /* hwtimer device */
    LPTIM_Module*   timer_periph; /* LPTIM peripheral base address */
    LPTIM_InitType* timer_init;   /* LPTIM initialization structure */
};

#ifdef __cplusplus
}
#endif

#endif /* __DRV_LPTIMER_H__ */