#include <drv_pm.h>

#ifdef RT_USING_PM

#if defined(BSP_USING_LPTIMER)
#include <rtdevice.h>

#define LPTIM_REG_MAX_VALUE (0xFFFF)

#define SRAM1_START_ADDR    (0x20000000)
#define SRAM1_SIZE          (1024 * 24)

#define SRAM2_START_ADDR    (0x20006000)
#define SRAM2_SIZE          (1024 * 8)

rt_weak void n32_pm_device_run(struct rt_pm* pm, uint8_t mode)
{
    /*todo add your code here*/
}

void sram2_reset(void)
{
    uint8_t* p_sram2 = (uint8_t*)SRAM2_START_ADDR;
    uint32_t i       = 0;
    for (i = 0; i < SRAM2_SIZE; i++) {
        *(p_sram2 + i) = 0;
    }
}

/**
 * @brief  Configures system clock after wake-up from low power mode: enable HSE, PLL
 *         and select PLL as system clock source.
 * @param  freq: PLL clock eg 108000000
 * @param  src
 *   This parameter can be one of the following values:
 *     @arg SYSCLK_PLLSRC_HSI,
 *     @arg SYSCLK_PLLSRC_HSIDIV2,
 *     @arg SYSCLK_PLLSRC_HSI_PLLDIV2,
 *     @arg SYSCLK_PLLSRC_HSIDIV2_PLLDIV2,
 *     @arg SYSCLK_PLLSRC_HSE,
 *     @arg SYSCLK_PLLSRC_HSEDIV2,
 *     @arg SYSCLK_PLLSRC_HSE_PLLDIV2,
 *     @arg SYSCLK_PLLSRC_HSEDIV2_PLLDIV2,
 */
static void set_sysclock_to_pll(uint32_t freq, SYSCLK_PLL_TYPE src)
{
    uint32_t    pllsrcclk = 0;
    uint32_t    pllsrc    = 0;
    uint32_t    pllmul    = 0;
    uint32_t    plldiv    = RCC_PLLDIVCLK_DISABLE;
    uint32_t    latency   = 0;
    uint32_t    pclk1div = 0, pclk2div = 0;
    uint32_t    msi_ready_flag   = RESET;
    ErrorStatus HSEStartUpStatus = ERROR;
    ErrorStatus HSIStartUpStatus = ERROR;

    if (HSE_VALUE != 8000000) {
        /* HSE_VALUE == 8000000 is needed in this project! */
        while (1);
    }

    /* SYSCLK, HCLK, PCLK2 and PCLK1 configuration
     * -----------------------------*/

    if ((src == SYSCLK_PLLSRC_HSI) || (src == SYSCLK_PLLSRC_HSIDIV2) || (src == SYSCLK_PLLSRC_HSI_PLLDIV2) || (src == SYSCLK_PLLSRC_HSIDIV2_PLLDIV2)) {
        /* Enable HSI */
        RCC_ConfigHsi(RCC_HSI_ENABLE);

        /* Wait till HSI is ready */
        HSIStartUpStatus = RCC_WaitHsiStable();

        if (HSIStartUpStatus != SUCCESS) {
            /* If HSI fails to start-up, the application will have wrong clock
               configuration. User can add here some code to deal with this
               error */

            /* Go to infinite loop */
            while (1);
        }

        if ((src == SYSCLK_PLLSRC_HSIDIV2) || (src == SYSCLK_PLLSRC_HSIDIV2_PLLDIV2)) {
            pllsrc    = RCC_PLL_HSI_PRE_DIV2;
            pllsrcclk = HSI_VALUE / 2;

            if (src == SYSCLK_PLLSRC_HSIDIV2_PLLDIV2) {
                plldiv    = RCC_PLLDIVCLK_ENABLE;
                pllsrcclk = HSI_VALUE / 4;
            }
        } else if ((src == SYSCLK_PLLSRC_HSI) || (src == SYSCLK_PLLSRC_HSI_PLLDIV2)) {
            pllsrc    = RCC_PLL_HSI_PRE_DIV1;
            pllsrcclk = HSI_VALUE;

            if (src == SYSCLK_PLLSRC_HSI_PLLDIV2) {
                plldiv    = RCC_PLLDIVCLK_ENABLE;
                pllsrcclk = HSI_VALUE / 2;
            }
        }

    } else if ((src == SYSCLK_PLLSRC_HSE) || (src == SYSCLK_PLLSRC_HSEDIV2) || (src == SYSCLK_PLLSRC_HSE_PLLDIV2) || (src == SYSCLK_PLLSRC_HSEDIV2_PLLDIV2)) {
        /* Enable HSE */
        RCC_ConfigHse(RCC_HSE_ENABLE);

        /* Wait till HSE is ready */
        HSEStartUpStatus = RCC_WaitHseStable();

        if (HSEStartUpStatus != SUCCESS) {
            /* If HSE fails to start-up, the application will have wrong clock
               configuration. User can add here some code to deal with this
               error */

            /* Go to infinite loop */
            while (1);
        }

        if ((src == SYSCLK_PLLSRC_HSEDIV2) || (src == SYSCLK_PLLSRC_HSEDIV2_PLLDIV2)) {
            pllsrc    = RCC_PLL_SRC_HSE_DIV2;
            pllsrcclk = HSE_VALUE / 2;

            if (src == SYSCLK_PLLSRC_HSEDIV2_PLLDIV2) {
                plldiv    = RCC_PLLDIVCLK_ENABLE;
                pllsrcclk = HSE_VALUE / 4;
            }
        } else if ((src == SYSCLK_PLLSRC_HSE) || (src == SYSCLK_PLLSRC_HSE_PLLDIV2)) {
            pllsrc    = RCC_PLL_SRC_HSE_DIV1;
            pllsrcclk = HSE_VALUE;

            if (src == SYSCLK_PLLSRC_HSE_PLLDIV2) {
                plldiv    = RCC_PLLDIVCLK_ENABLE;
                pllsrcclk = HSE_VALUE / 2;
            }
        }
    }

    latency = (freq / 32000000);

    if (freq > 54000000) {
        pclk1div = RCC_HCLK_DIV4;
        pclk2div = RCC_HCLK_DIV2;
    } else {
        if (freq > 27000000) {
            pclk1div = RCC_HCLK_DIV2;
            pclk2div = RCC_HCLK_DIV1;
        } else {
            pclk1div = RCC_HCLK_DIV1;
            pclk2div = RCC_HCLK_DIV1;
        }
    }

    if (((freq % pllsrcclk) == 0) && ((freq / pllsrcclk) >= 2) && ((freq / pllsrcclk) <= 32)) {
        pllmul = (freq / pllsrcclk);
        if (pllmul <= 16) {
            pllmul = ((pllmul - 2) << 18);
        } else {
            pllmul = (((pllmul - 17) << 18) | (1 << 27));
        }
    } else {
        /* User can add here some code to deal with this error */
        while (1);
    }

    /* Cheak if MSI is Ready */
    if (RESET == RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_MSIRD)) {
        /* Enable MSI and Config Clock */
        RCC_ConfigMsi(RCC_MSI_ENABLE, RCC_MSI_RANGE_4M);
        /* Waits for MSI start-up */
        while (SUCCESS != RCC_WaitMsiStable());

        msi_ready_flag = SET;
    }

    /* Select MSI as system clock source */
    RCC_ConfigSysclk(RCC_SYSCLK_SRC_MSI);

    FLASH_SetLatency(latency);

    /* HCLK = SYSCLK */
    RCC_ConfigHclk(RCC_SYSCLK_DIV1);

    /* PCLK2 = HCLK */
    RCC_ConfigPclk2(pclk2div);

    /* PCLK1 = HCLK */
    RCC_ConfigPclk1(pclk1div);

    /* Disable PLL */
    RCC_EnablePll(DISABLE);

    RCC_ConfigPll(pllsrc, pllmul, plldiv);

    /* Enable PLL */
    RCC_EnablePll(ENABLE);

    /* Wait till PLL is ready */
    while (RCC_GetFlagStatus(RCC_CTRL_FLAG_PLLRDF) == RESET); /* If this flag is always not set, it means PLL is failed, user can add here some code to deal with this error */

    /* Select PLL as system clock source */
    RCC_ConfigSysclk(RCC_SYSCLK_SRC_PLLCLK);

    /* Wait till PLL is used as system clock source */
    while (RCC_GetSysclkSrc() != 0x0C); /* If this bits are always not set, it means system clock select PLL failed, user can add here some code to deal with this error */

    if (msi_ready_flag == SET) {
        /* MSI oscillator OFF */
        RCC_ConfigMsi(RCC_MSI_DISABLE, RCC_MSI_RANGE_4M);
    }
}

static rt_tick_t n32_os_tick_from_pm_tick(rt_uint32_t tick)
{
    static rt_uint32_t os_tick_remain = 0;
    rt_tick_t          os_tick        = 0;
    rt_uint32_t        freq           = 1000;

    os_tick = (tick * RT_TICK_PER_SECOND + os_tick_remain) / freq;

    os_tick_remain += (tick * RT_TICK_PER_SECOND);
    os_tick_remain %= freq;

    return os_tick;
}

static rt_tick_t n32_pm_tick_from_os_tick(rt_tick_t tick)
{
    rt_uint32_t freq = 1000;
    return (tick * freq / RT_TICK_PER_SECOND);
}

static void sleep(struct rt_pm* pm, uint8_t mode)
{
    switch (mode) {
        case PM_SLEEP_MODE_NONE:
            break;

        case PM_SLEEP_MODE_IDLE:
            break;

        case PM_SLEEP_MODE_LIGHT:
            PWR_EnterSLEEPMode(SLEEP_OFF_EXIT, PWR_SLEEPENTRY_WFI);
            break;

        case PM_SLEEP_MODE_DEEP:
            PWR_EnterSTOP2Mode(PWR_STOPENTRY_WFI, PWR_CTRL3_RAM1RET);
            /*Reset SRAM2 when wake up from stop2 mode*/
            sram2_reset();
            /*multiply System Clock Frequency*/
            set_sysclock_to_pll(SystemCoreClock, SYSCLK_PLLSRC_HSE_PLLDIV2);
            break;

        case PM_SLEEP_MODE_STANDBY:
            PWR_EnterSTANDBYMode(PWR_STOPENTRY_WFI, PWR_CTRL3_RAM2RET);
            break;

        case PM_SLEEP_MODE_SHUTDOWN:

            break;

        default:
            break;
    }
}

static void pm_timer_start(struct rt_pm* pm, rt_uint32_t timeout)
{
    RT_ASSERT(pm != RT_NULL);
    RT_ASSERT(timeout > 0);

    LPTIM_Disable(LPTIM);
    while (LPTIM_IsEnabled(LPTIM));
    if (timeout != RT_TICK_MAX) {
        rt_uint32_t max_tick = LPTIM_REG_MAX_VALUE;

        /* Convert OS Tick to pmtimer timeout value */
        timeout = n32_pm_tick_from_os_tick(timeout);

        if (timeout > max_tick) {
            timeout = max_tick;
        }

        /* Enable interrupt   */
        LPTIM_SetPrescaler(LPTIM, LPTIM_PRESCALER_DIV1);
        LPTIM_EnableIT_CMPM(LPTIM);
        /* config lptim ARR and compare register */
        LPTIM_Enable(LPTIM);
        LPTIM_SetAutoReload(LPTIM, timeout);
        LPTIM_SetCompare(LPTIM, 0);
        LPTIM_StartCounter(LPTIM, LPTIM_OPERATING_MODE_ONESHOT);
    }
}

static void pm_timer_stop(struct rt_pm* pm)
{
    LPTIM_Disable(LPTIM);
    LPTIM_DisableIT_CMPM(LPTIM);
}

static rt_tick_t pm_timer_get_tick(struct rt_pm* pm)
{
    rt_uint32_t timer_tick;
    timer_tick = LPTIM_GetCounter(LPTIM);
    return n32_os_tick_from_pm_tick(timer_tick);
}

static const struct rt_pm_ops _ops = {
    sleep,
    n32_pm_device_run,
    pm_timer_start,
    pm_timer_stop,
    pm_timer_get_tick,
};

void LPTIM_WKUP_IRQHandler(void)
{
    if (LPTIM_IsActiveFlag_CMPM(LPTIM) != RESET) {

        LPTIM_ClearFLAG_CMPM(LPTIM);
        EXTI_ClrITPendBit(EXTI_LINE24);
    }
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

int rt_pm_hw_init(void)
{
    rt_uint8_t timer_mask = 0;

    /* Enable Power Clock */
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);

    /* initialize timer mask */
    timer_mask = 1UL << PM_SLEEP_MODE_DEEP;

    /* initialize system pm module */
    rt_system_pm_init(&_ops, timer_mask, RT_NULL);

    /* initialize lptimer */
    RCC_EnableLsi(ENABLE);
    LPTIMNVIC_Config(ENABLE);
    RCC_ConfigLPTIMClk(RCC_LPTIMCLK_SRC_LSI);
    RCC_EnableRETPeriphClk(RCC_RET_PERIPH_LPTIM, ENABLE);

    return 0;
}

INIT_CORE_EXPORT(rt_pm_hw_init);

#endif /* BSP_USING_LPTIMER */
#endif /* RT_USING_PM */