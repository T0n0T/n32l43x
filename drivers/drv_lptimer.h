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

typedef enum {
    DRV_HW_LPTIMER_CTRL_GET_FREQ     = 0X21, /* get the timer frequency*/
} drv_hw_lptimer_ctrl_t;

struct n32_lptimer {
    rt_hwtimer_t   time_device;  /* hwtimer device */
    LPTIM_Module*  timer_periph; /* LPTIM peripheral base address */
    LPTIM_InitType timer_init;   /* LPTIM initialization structure */
};

#ifdef __cplusplus
}
#endif

#endif /* __DRV_LPTIMER_H__ */