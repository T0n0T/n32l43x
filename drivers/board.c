#include <stdio.h>
#include <stdint.h>
#include "elog.h"
#include "board.h"
#include "n32l43x_lptim.h"
#include "n32l43x_lpuart.h"

ErrorStatus SetSysClockToMSI(void);
ErrorStatus SetSysClockToHSI(void);
ErrorStatus SetSysClockToHSE(void);
ErrorStatus SetSysClockToPLL(uint32_t freq, uint8_t src);

void board_init(void)
{
    SetSysClockToHSI();
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);
}

void dump_clk(void)
{
    RCC_ClocksType RCC_ClockFreq;
    RCC_GetClocksFreqValue(&RCC_ClockFreq);
    printf("\r\nSYSCLK: %u\r\n", (unsigned int)RCC_ClockFreq.SysclkFreq);
    printf("HCLK: %u\r\n", (unsigned int)RCC_ClockFreq.HclkFreq);
    printf("PCLK1: %u\r\n", (unsigned int)RCC_ClockFreq.Pclk1Freq);
    printf("PCLK2: %u\r\n", (unsigned int)RCC_ClockFreq.Pclk2Freq);
}

/**
 * @brief  Selects MSI as System clock source and configure HCLK, PCLK2
 *         and PCLK1 prescalers.
 */
ErrorStatus SetSysClockToMSI(void)
{
    uint32_t timeout_value = 0xFFFFFF;

    if (RESET == RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_MSIRD)) {
        /* Enable MSI and Config Clock */
        RCC_ConfigMsi(RCC_MSI_ENABLE, RCC_MSI_RANGE_4M);
        /* Waits for MSI start-up */
        if (SUCCESS != RCC_WaitMsiStable()) {
            return ERROR;
        }
    }

    /* Enable Prefetch Buffer */
    FLASH_PrefetchBufSet(FLASH_PrefetchBuf_EN);

    /* Select MSI as system clock source */
    RCC_ConfigSysclk(RCC_SYSCLK_SRC_MSI);

    /* Wait till MSI is used as system clock source */
    while (RCC_GetSysclkSrc() != 0x00) {
        if ((timeout_value--) == 0) {
            return ERROR;
        }
    }

    /* Flash 0 wait state */
    FLASH_SetLatency(FLASH_LATENCY_0);

    /* HCLK = SYSCLK */
    RCC_ConfigHclk(RCC_SYSCLK_DIV1);

    /* PCLK2 = HCLK */
    RCC_ConfigPclk2(RCC_HCLK_DIV1);

    /* PCLK1 = HCLK */
    RCC_ConfigPclk1(RCC_HCLK_DIV1);

    return SUCCESS;
}

/**
 * @brief  Selects HSI as System clock source and configure HCLK, PCLK2
 *         and PCLK1 prescalers.
 */
ErrorStatus SetSysClockToHSI(void)
{
    uint32_t    msi_ready_flag   = RESET;
    uint32_t    timeout_value    = 0xFFFFFF;
    ErrorStatus HSIStartUpStatus = ERROR;

    RCC_EnableHsi(ENABLE);

    /* Wait till HSI is ready */
    HSIStartUpStatus = RCC_WaitHsiStable();

    if (HSIStartUpStatus == SUCCESS) {
        /* Enable Prefetch Buffer */
        FLASH_PrefetchBufSet(FLASH_PrefetchBuf_EN);

        if (((*(__IO uint8_t*)((UCID_BASE + 0x2))) == 0x01) || ((*(__IO uint8_t*)((UCID_BASE + 0x2))) == 0x11) || ((*(__IO uint8_t*)((UCID_BASE + 0x2))) == 0xFF)) {
            /* Cheak if MSI is Ready */
            if (RESET == RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_MSIRD)) {
                /* Enable MSI and Config Clock */
                RCC_ConfigMsi(RCC_MSI_ENABLE, RCC_MSI_RANGE_4M);
                /* Waits for MSI start-up */
                if (SUCCESS != RCC_WaitMsiStable()) {
                    return ERROR;
                }

                msi_ready_flag = SET;
            }

            /* Select MSI as system clock source */
            RCC_ConfigSysclk(RCC_SYSCLK_SRC_MSI);

            /* Disable PLL */
            RCC_EnablePll(DISABLE);

            RCC_ConfigPll(RCC_PLL_HSI_PRE_DIV2, RCC_PLL_MUL_2, RCC_PLLDIVCLK_DISABLE);

            /* Enable PLL */
            RCC_EnablePll(ENABLE);

            /* Wait till PLL is ready */
            while (RCC_GetFlagStatus(RCC_CTRL_FLAG_PLLRDF) == RESET) {
                if ((timeout_value--) == 0) {
                    return ERROR;
                }
            }

            /* Select PLL as system clock source */
            RCC_ConfigSysclk(RCC_SYSCLK_SRC_PLLCLK);

            /* Wait till PLL is used as system clock source */
            timeout_value = 0xFFFFFF;
            while (RCC_GetSysclkSrc() != 0x0C) {
                if ((timeout_value--) == 0) {
                    return ERROR;
                }
            }

            if (msi_ready_flag == SET) {
                /* MSI oscillator OFF */
                RCC_ConfigMsi(RCC_MSI_DISABLE, RCC_MSI_RANGE_4M);
            }
        } else {
            /* Select HSI as system clock source */
            RCC_ConfigSysclk(RCC_SYSCLK_SRC_HSI);

            /* Wait till HSI is used as system clock source */
            timeout_value = 0xFFFFFF;
            while (RCC_GetSysclkSrc() != 0x04) {
                if ((timeout_value--) == 0) {
                    return ERROR;
                }
            }
        }

        /* Flash 0 wait state */
        FLASH_SetLatency(FLASH_LATENCY_0);

        /* HCLK = SYSCLK */
        RCC_ConfigHclk(RCC_SYSCLK_DIV1);

        /* PCLK2 = HCLK */
        RCC_ConfigPclk2(RCC_HCLK_DIV1);

        /* PCLK1 = HCLK */
        RCC_ConfigPclk1(RCC_HCLK_DIV1);
    } else {
        /* If HSI fails to start-up, the application will have wrong clock
           configuration. User can add here some code to deal with this error */
        return ERROR;
    }
    return SUCCESS;
}

/**
 * @brief  Selects HSE as System clock source and configure HCLK, PCLK2
 *         and PCLK1 prescalers.
 */
ErrorStatus SetSysClockToHSE(void)
{
    uint32_t timeout_value = 0xFFFFFF;
    /* SYSCLK, HCLK, PCLK2 and PCLK1 configuration
     * -----------------------------*/

    uint32_t    msi_ready_flag   = RESET;
    ErrorStatus HSEStartUpStatus = ERROR;

    /* Enable HSE */
    RCC_ConfigHse(RCC_HSE_ENABLE);

    /* Wait till HSE is ready */
    HSEStartUpStatus = RCC_WaitHseStable();

    if (HSEStartUpStatus == SUCCESS) {
        /* Enable Prefetch Buffer */
        FLASH_PrefetchBufSet(FLASH_PrefetchBuf_EN);

        if (((*(__IO uint8_t*)((UCID_BASE + 0x2))) == 0x01) || ((*(__IO uint8_t*)((UCID_BASE + 0x2))) == 0x11) || ((*(__IO uint8_t*)((UCID_BASE + 0x2))) == 0xFF)) {
            /* Cheak if MSI is Ready */
            if (RESET == RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_MSIRD)) {
                /* Enable MSI and Config Clock */
                RCC_ConfigMsi(RCC_MSI_ENABLE, RCC_MSI_RANGE_4M);
                /* Waits for MSI start-up */
                if (SUCCESS != RCC_WaitMsiStable()) {
                    return ERROR;
                }

                msi_ready_flag = SET;
            }

            /* Select MSI as system clock source */
            RCC_ConfigSysclk(RCC_SYSCLK_SRC_MSI);

            /* Disable PLL */
            RCC_EnablePll(DISABLE);

            RCC_ConfigPll(RCC_PLL_SRC_HSE_DIV2, RCC_PLL_MUL_2, RCC_PLLDIVCLK_DISABLE);

            /* Enable PLL */
            RCC_EnablePll(ENABLE);

            /* Wait till PLL is ready */
            while (RCC_GetFlagStatus(RCC_CTRL_FLAG_PLLRDF) == RESET) {
                if ((timeout_value--) == 0) {
                    return ERROR;
                }
            }

            /* Select PLL as system clock source */
            RCC_ConfigSysclk(RCC_SYSCLK_SRC_PLLCLK);

            /* Wait till PLL is used as system clock source */
            timeout_value = 0xFFFFFF;
            while (RCC_GetSysclkSrc() != 0x0C) {
                if ((timeout_value--) == 0) {
                    return ERROR;
                }
            }

            if (msi_ready_flag == SET) {
                /* MSI oscillator OFF */
                RCC_ConfigMsi(RCC_MSI_DISABLE, RCC_MSI_RANGE_4M);
            }
        } else {
            /* Select HSE as system clock source */
            RCC_ConfigSysclk(RCC_SYSCLK_SRC_HSE);

            /* Wait till HSE is used as system clock source */
            timeout_value = 0xFFFFFF;
            while (RCC_GetSysclkSrc() != 0x08) {
                if ((timeout_value--) == 0) {
                    return ERROR;
                }
            }
        }

        if (HSE_Value <= 32000000) {
            /* Flash 0 wait state */
            FLASH_SetLatency(FLASH_LATENCY_0);
        } else {
            /* Flash 1 wait state */
            FLASH_SetLatency(FLASH_LATENCY_1);
        }

        /* HCLK = SYSCLK */
        RCC_ConfigHclk(RCC_SYSCLK_DIV1);

        /* PCLK2 = HCLK */
        RCC_ConfigPclk2(RCC_HCLK_DIV1);

        /* PCLK1 = HCLK */
        RCC_ConfigPclk1(RCC_HCLK_DIV1);
    } else {
        /* If HSE fails to start-up, the application will have wrong clock
           configuration. User can add here some code to deal with this error */
        return ERROR;
    }
    return SUCCESS;
}

/**
 *\*\name    SetSysClockToPLL.
 *\*\fun     Selects PLL clock as System clock source and configure HCLK, PCLK2
 *\*\         and PCLK1 prescalers.
 *\*\param   none
 *\*\note    PLL frequency must be greater than or equal to 32MHz.
 *\*\return  none
 **/
ErrorStatus SetSysClockToPLL(uint32_t freq, uint8_t src)
{
    uint32_t    pllsrcclk;
    uint32_t    pllsrc;
    uint32_t    pllmul;
    uint32_t    plldiv = RCC_PLLDIVCLK_DISABLE;
    uint32_t    latency;
    uint32_t    pclk1div, pclk2div;
    uint32_t    msi_ready_flag   = RESET;
    uint32_t    timeout_value    = 0xFFFFFF;
    ErrorStatus HSIStartUpStatus = ERROR;
    ErrorStatus HSEStartUpStatus = ERROR;

    if (HSE_VALUE != 8000000) {
        /* HSE_VALUE == 8000000 is needed in this project! */
        return ERROR;
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
            return ERROR;
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
            return ERROR;
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
        return ERROR;
    }

    /* Cheak if MSI is Ready */
    if (RESET == RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_MSIRD)) {
        /* Enable MSI and Config Clock */
        RCC_ConfigMsi(RCC_MSI_ENABLE, RCC_MSI_RANGE_4M);
        /* Waits for MSI start-up */
        if (SUCCESS != RCC_WaitMsiStable()) {
            return ERROR;
        }

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
    while (RCC_GetFlagStatus(RCC_CTRL_FLAG_PLLRDF) == RESET) {
        if ((timeout_value--) == 0) {
            return ERROR;
        }
    }

    /* Select PLL as system clock source */
    RCC_ConfigSysclk(RCC_SYSCLK_SRC_PLLCLK);

    /* Wait till PLL is used as system clock source */
    timeout_value = 0xFFFFFF;
    while (RCC_GetSysclkSrc() != 0x0C) {
        if ((timeout_value--) == 0) {
            return ERROR;
        }
    }

    if (msi_ready_flag == SET) {
        /* MSI oscillator OFF */
        RCC_ConfigMsi(RCC_MSI_DISABLE, RCC_MSI_RANGE_4M);
    }
    return SUCCESS;
}

// void wakeup_pin_init(int wkup_pin, void (*cb)(void))
// {
//     uint32_t      clk   = 0;
//     uint16_t      pin   = 0;
//     GPIO_Module*  GPIOx = 0;
//     GPIO_InitType GPIO_InitStructure;
//     GPIO_InitStruct(&GPIO_InitStructure);
//     switch ((WAKEUP_PINX)wkup_pin) {
//         case WAKEUP_PIN0:
//             pin   = GPIO_PIN_0;
//             clk   = RCC_APB2_PERIPH_GPIOA;
//             GPIOx = GPIOA;
//             break;
//         case WAKEUP_PIN1:
//             pin = GPIO_PIN_8;
//             clk = RCC_APB2_PERIPH_GPIOA;
//             GPIOx = GPIOA;
//             break;
//         case WAKEUP_PIN2:
//             pin = GPIO_PIN_13;
//             clk = RCC_APB2_PERIPH_GPIOC;
//             GPIOx = GPIOC;
//             break;
//         default:
//             return;
//     }
//     GPIO_InitStructure.Pin       = pin;
//     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Input;
//     GPIO_InitStructure.GPIO_Pull = GPIO_Pull_Down;
//     RCC_EnableAPB2PeriphClk(clk, ENABLE);
//     GPIO_InitPeripheral(GPIOx, &GPIO_InitStructure);
//     PWR_WakeUpPinEnable(wkup_pin, ENABLE);
// }

static wakeup_handle_func handler;

void wakeup_pin_init(wakeup_handle_func h)
{
    GPIO_InitType GPIO_InitStructure;
    EXTI_InitType EXTI_InitStructure;
    NVIC_InitType NVIC_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);
    EXTI_InitStruct(&EXTI_InitStructure);

    /* Enable the GPIO Clock */
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOC, ENABLE);

    /*Configure the GPIO pin as input floating*/
    GPIO_InitStructure.Pin       = GPIO_PIN_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Input;
    GPIO_InitStructure.GPIO_Pull = GPIO_No_Pull;
    GPIO_InitPeripheral(GPIOC, &GPIO_InitStructure);

    /*Configure key EXTI Line to key input  Pin*/
    GPIO_ConfigEXTILine(GPIOC_PORT_SOURCE, GPIO_PIN_SOURCE13);

    /*Configure key EXTI line*/
    EXTI_InitStructure.EXTI_Line    = EXTI_LINE13;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStructure);

    /*Set key input interrupt priority*/
    NVIC_InitStructure.NVIC_IRQChannel                   = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;

    NVIC_Init(&NVIC_InitStructure);
}

void EXTI15_10_IRQHandler(void)
{
    if (RESET != EXTI_GetITStatus(EXTI_LINE3)) {
        uint8_t bit = GPIO_ReadInputDataBit(GPIOC, GPIO_PIN_13);
        if (handler != NULL) {
            handler(bit);
        }
        EXTI_ClrITPendBit(EXTI_LINE13);
    }
}
