#include "drv_lptimer.h"

#ifdef RT_USING_HWTIMER
#if defined(BSP_USING_LPTIMER)
#include <rtdevice.h>

rt_hwtimer_t lptimer = {0};

static const struct rt_hwtimer_info n32_lptimer_info = {
    1000000, /* the maximum count frequency can be set */
    100,    /* the minimum count frequency can be set */
    0xFFFF,
    HWTIMER_CNTMODE_UP,
};

void LPTIM_WKUP_IRQHandler(void)
{
    rt_interrupt_enter();
    if (LPTIM_IsActiveFlag_CMPM(LPTIM) != RESET) {
        LPTIM_ClearFLAG_CMPM(LPTIM);
        EXTI_ClrITPendBit(EXTI_LINE24);
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

    /* Enable the RTC Alarm Interrupt */
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
        RCC_EnableLsi(ENABLE);
        RCC_ConfigLPTIMClk(RCC_LPTIMCLK_SRC_LSI);
        RCC_EnableRETPeriphClk(RCC_RET_PERIPH_LPTIM, ENABLE);

        /* Initialize LPTIM */
        LPTIM_StructInit(lptimer->timer_init);
        lptimer->timer_init->ClockSource = LPTIM_CLK_SOURCE_INTERNAL;
        lptimer->timer_init->Prescaler   = LPTIM_PRESCALER_DIV1;
        lptimer->timer_init->Waveform    = LPTIM_OUTPUT_WAVEFORM_PWM;
        lptimer->timer_init->Polarity    = LPTIM_OUTPUT_POLARITY_REGULAR;

        LPTIM_Init(lptimer->timer_periph, lptimer->timer_init);
        LPTIMNVIC_Config(ENABLE);
        LPTIM_SetPrescaler(lptimer->timer_periph, LPTIM_PRESCALER_DIV1);
        LPTIM_EnableIT_ARRM(lptimer->timer_periph);
        LPTIM_Enable(lptimer->timer_periph);
    } else {
        LPTIM_DeInit(lptimer->timer_periph);
    }
}

static rt_err_t n32_lptimer_start(rt_hwtimer_t* timer, rt_uint32_t cnt, rt_hwtimer_mode_t mode)
{
    struct n32_lptimer* lptimer = (struct n32_lptimer*)timer;

    /* Set autoreload value */
    LPTIM_SetAutoReload(lptimer->timer_periph, cnt - 1);

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
        case HWTIMER_CTRL_FREQ_SET:
            /* Check if timer is running */
            was_running = LPTIM_IsEnabled(lptimer->timer_periph);

            /* Stop timer if running */
            if (was_running) {
                LPTIM_Disable(lptimer->timer_periph);
            }

            /* Set timer frequency */
            {
                // rt_uint32_t freq = *(rt_uint32_t *)args;
                // rt_uint32_t prescaler = 0;

                // /* Calculate prescaler value */
                // if (freq <= 2000) prescaler = LPTIM_PRESCALER_DIV128;
                // else if (freq <= 4000) prescaler = LPTIM_PRESCALER_DIV64;
                // else if (freq <= 8000) prescaler = LPTIM_PRESCALER_DIV32;
                // else if (freq <= 16000) prescaler = LPTIM_PRESCALER_DIV16;
                // else if (freq <= 32000) prescaler = LPTIM_PRESCALER_DIV8;
                // else if (freq <= 64000) prescaler = LPTIM_PRESCALER_DIV4;
                // else if (freq <= 128000) prescaler = LPTIM_PRESCALER_DIV2;
                // else prescaler = LPTIM_PRESCALER_DIV1;

                LPTIM_SetPrescaler(lptimer->timer_periph, LPTIM_PRESCALER_DIV1);
            }

            /* Restart timer if it was running */
            if (was_running) {
                LPTIM_Enable(lptimer->timer_periph);
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
    int i      = 0;
    int result = RT_EOK;

    lptimer.info = &n32_lptimer_info;
    lptimer.ops  = &n32_lptimer_ops;
    rt_device_hwtimer_register(&lptimer, "lptimer", NULL);

    return result;
}
INIT_DEVICE_EXPORT(rt_hw_lptimer_init);

#endif /* defined(BSP_USING_HWTIMERx) */
#endif /* RT_USING_HWTIMER */