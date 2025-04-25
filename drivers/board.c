#include <rthw.h>
#include <rtthread.h>
#include "n32l40x.h"
#include "board.h"

/**
 * @brief  Configures Vector Table base location.
 */
void nvic_configuration(void)
{
#ifdef VECT_TAB_RAM
    /* Set the Vector Table base location at 0x20000000 */
    NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);
#else /* VECT_TAB_FLASH  */
    /* Set the Vector Table base location at 0x08000000 */
    NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
#endif
}

void set_sysclock_to_pll(uint32_t freq, SYSCLK_PLL_TYPE src)
{
    uint32_t    pllsrcclk;
    uint32_t    pllsrc;
    uint32_t    pllmul;
    uint32_t    plldiv = RCC_PLLDIVCLK_DISABLE;
    uint32_t    latency;
    uint32_t    pclk1div, pclk2div;
    uint32_t    msi_ready_flag = RESET;
    ErrorStatus HSEStartUpStatus;
    ErrorStatus HSIStartUpStatus;

    if (HSE_VALUE != 8000000) {
        /* HSE_VALUE == 8000000 is needed in this project, if HSE crystal is not 8MHz, User should modify HSE_VALUE ! */
        while (1);
    }

    /* SYSCLK, HCLK, PCLK2 and PCLK1 configuration */

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
        /* Cannot make a PLL multiply factor to freq. */
        // log_info("Cannot make a PLL multiply factor to freq..\n");
        while (1); /* User can add here some code to deal with this error */
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
    while (RCC_GetFlagStatus(RCC_CTRL_FLAG_PLLRDF) == RESET); /* if this flag is always not set, it means PLL is failed, User can add here some code to deal with this error*/

    /* Select PLL as system clock source */
    RCC_ConfigSysclk(RCC_SYSCLK_SRC_PLLCLK);

    /* Wait till PLL is used as system clock source */
    while (RCC_GetSysclkSrc() != 0x0C); /* if this bits is always not set, it means select PLL as system clock failed, User can add here some code to deal with this error*/

    if (msi_ready_flag == SET) {
        /* MSI oscillator OFF */
        RCC_ConfigMsi(RCC_MSI_DISABLE, RCC_MSI_RANGE_4M);
    }

    /* Configure the SysTick */
    SysTick_Config(freq / RT_TICK_PER_SECOND); /* 1ms */

    RCC_ClocksType RCC_ClockFreq;
    RCC_GetClocksFreqValue(&RCC_ClockFreq);
    // rt_kprintf("SYSCLK: %u\n", (unsigned int)RCC_ClockFreq.SysclkFreq);
    // rt_kprintf("HCLK: %u\n", (unsigned int)RCC_ClockFreq.HclkFreq);
    // rt_kprintf("PCLK1: %u\n", (unsigned int)RCC_ClockFreq.Pclk1Freq);
    // rt_kprintf("PCLK2: %u\n", (unsigned int)RCC_ClockFreq.Pclk2Freq);
}

#ifdef DEBUG
#include <n32l40x_dbg.h>
#endif

void rt_hw_board_init()
{
    /* NVIC Configuration */
    nvic_configuration();
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);
    set_sysclock_to_pll(SystemCoreClock, SYSCLK_PLLSRC_HSE_PLLDIV2);

#ifdef DEBUG
    DBG_ConfigPeriph(DBG_SLEEP | DBG_STOP | DBG_STDBY, ENABLE);
#endif

#ifdef RT_USING_HEAP
    rt_system_heap_init((void*)HEAP_START, (void*)HEAP_END);
#endif

#ifdef RT_USING_PIN
    extern int rt_hw_pin_init(void);
    rt_hw_pin_init();
#endif

#ifdef RT_USING_SERIAL
#if defined(BSP_USING_LPUART)
    extern int rt_hw_lpuart_init(void);
    rt_hw_lpuart_init();
#ifdef RT_USING_CONSOLE
    rt_console_set_device("lpuart");
#endif
#elif defined(BSP_USING_USART1)
    extern int rt_hw_usart_init(void);
    rt_hw_usart_init();
#ifdef RT_USING_CONSOLE
    rt_console_set_device("usart1");
#endif
#endif
#endif

/* Call components board initial (use INIT_BOARD_EXPORT()) */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}

void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}
