#include "drv_lptimer.h"

#ifdef RT_USING_HWTIMER
#if defined(BSP_USING_LPTIMER)
#include <rtdevice.h>
#define LOG_TAG "drv.lptimer"

struct n32_lptimer _lptimer = {0};

static const struct rt_hwtimer_info n32_lptimer_info = {
    32768, /* the maximum count frequency can be set */
    32768, /* the minimum count frequency can be set */
    0xFFFF,
    HWTIMER_CNTMODE_UP,
};

void LPTIM_WKUP_IRQHandler(void)
{
    rt_interrupt_enter();
    if (LPTIM_IsActiveFlag_ARRM(LPTIM) != RESET) {
        LPTIM_ClearFLAG_ARRM(LPTIM);
        EXTI_ClrITPendBit(EXTI_LINE24);
        LPTIM_Disable(LPTIM);
    }
    rt_interrupt_leave();
}

void LPTIMNVIC_Config(FunctionalState Cmd)
{
    EXTI_InitType EXTI_InitStructure;
    NVIC_InitType NVIC_InitStructure;
    EXTI_InitStruct(&EXTI_InitStructure);

    EXTI_ClrITPendBit(EXTI_LINE24);
    EXTI_InitStructure.EXTI_Line = EXTI_LINE24;
#ifdef __TEST_SEVONPEND_WFE_NVIC_DIS__
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Event;
#else
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
#endif
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel                   = LPTIM_WKUP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = Cmd;
    NVIC_Init(&NVIC_InitStructure);
}

static void n32_lptimer_init(rt_hwtimer_t* timer, rt_uint32_t state)
{
    struct n32_lptimer* lptimer = (struct n32_lptimer*)timer;

    if (state) {
        /* Enable LPTIM clock as 32.768KHz */
        RCC_EnableLsi(ENABLE);
        RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);
        RCC_ConfigLPTIMClk(RCC_LPTIMCLK_SRC_LSI);
        RCC_EnableRETPeriphClk(RCC_RET_PERIPH_LPTIM, ENABLE);

        /* Initialize LPTIM */
        LPTIMNVIC_Config(ENABLE);
        LPTIM_SetPrescaler(LPTIM, LPTIM_PRESCALER_DIV1);
        LPTIM_Init(lptimer->timer_periph, &lptimer->timer_init);
    } else {
        LPTIM_DeInit(lptimer->timer_periph);
    }
}

static rt_err_t n32_lptimer_start(rt_hwtimer_t* timer, rt_uint32_t cnt, rt_hwtimer_mode_t mode)
{
    struct n32_lptimer* lptimer = (struct n32_lptimer*)timer;

    LPTIM_EnableIT_ARRM(lptimer->timer_periph);
    LPTIM_Enable(lptimer->timer_periph);

    /* Set autoreload value */
    LPTIM_SetAutoReload(lptimer->timer_periph, cnt);

    /* Start timer in selected mode */
    if (mode == HWTIMER_MODE_ONESHOT) {
        LPTIM_StartCounter(lptimer->timer_periph, LPTIM_OPERATING_MODE_ONESHOT);
    } else {
        LPTIM_StartCounter(lptimer->timer_periph, LPTIM_OPERATING_MODE_CONTINUOUS);
    }
    return RT_EOK;
}

static void n32_lptimer_stop(rt_hwtimer_t* timer)
{
    struct n32_lptimer* lptimer = (struct n32_lptimer*)timer;
    LPTIM_Disable(lptimer->timer_periph);
}

static rt_uint32_t n32_lptimer_count_get(rt_hwtimer_t* timer)
{
    struct n32_lptimer* lptimer = (struct n32_lptimer*)timer;
    return LPTIM_GetCounter(lptimer->timer_periph);
}

static rt_err_t n32_lptimer_control(rt_hwtimer_t* timer, rt_uint32_t cmd, void* args)
{
    struct n32_lptimer* lptimer     = (struct n32_lptimer*)timer;
    rt_bool_t           was_running = RT_FALSE;

    switch (cmd) {
        case DRV_HW_LPTIMER_CTRL_GET_FREQ:
            /* timer default frequency as 32768 Hz */
            if (args != RT_NULL) {
                *(rt_uint32_t*)args = 32768;
            }
            break;
        case HWTIMER_CTRL_STOP:
            /* Stop timer */
            LPTIM_Disable(lptimer->timer_periph);
            break;
        case HWTIMER_CTRL_INFO_GET:
            /* Get timer information */
            if (args != RT_NULL) {
                *(struct rt_hwtimer_info*)args = n32_lptimer_info;
            }
            break;
        default:
            return -RT_ERROR;
    }

    return RT_EOK;
}

static const struct rt_hwtimer_ops n32_lptimer_ops = {
    .init      = n32_lptimer_init,
    .start     = n32_lptimer_start,
    .stop      = n32_lptimer_stop,
    .count_get = n32_lptimer_count_get,
    .control   = n32_lptimer_control,
};

int rt_hw_lptimer_init(void)
{
    int result                = RT_EOK;
    _lptimer.timer_periph     = LPTIM;
    _lptimer.time_device.info = &n32_lptimer_info;
    _lptimer.time_device.ops  = &n32_lptimer_ops;
    rt_device_hwtimer_register(&_lptimer.time_device, "lptimer", NULL);

    return result;
}
INIT_BOARD_EXPORT(rt_hw_lptimer_init);

#endif /* defined(BSP_USING_HWTIMERx) */
#endif /* RT_USING_HWTIMER */