#include <stdio.h>
#include <stdint.h>
#include "elog.h"
#include "board.h"
#include "n32l43x_lptim.h"
#include "n32l43x_lpuart.h"

void board_init(void)
{
    SetSysClockToPLL(SystemCoreClock, SYSCLK_PLLSRC_HSE_PLLDIV2);
    SystemCoreClockUpdate();
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);
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

static wakeup_handle_func wakeup_handler;

void EXTI1_IRQHandler(void)
{
    if (RESET != EXTI_GetITStatus(EXTI_LINE1)) {
        uint8_t bit = GPIO_ReadInputDataBit(GPIOB, GPIO_PIN_1);
        if (wakeup_handler != NULL) {
            wakeup_handler(WAKE_SRC_RFID, bit);
        }
        EXTI_ClrITPendBit(EXTI_LINE1);
    }
}

void EXTI15_10_IRQHandler(void)
{
    if (RESET != EXTI_GetITStatus(EXTI_LINE13)) {
        uint8_t bit = GPIO_ReadInputDataBit(GPIOC, GPIO_PIN_13);
        if (wakeup_handler != NULL) {
            wakeup_handler(WAKE_SRC_KEY, bit);
        }
        EXTI_ClrITPendBit(EXTI_LINE13);
    }
}

void wakeup_init(wakeup_handle_func h)
{
    GPIO_InitType GPIO_InitStructure;
    EXTI_InitType EXTI_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);
    EXTI_InitStruct(&EXTI_InitStructure);

    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOB | RCC_APB2_PERIPH_GPIOC | RCC_APB2_PERIPH_AFIO, ENABLE);

    /*Configure the GPIO pin as input floating*/
    GPIO_InitStructure.Pin       = GPIO_PIN_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pull = GPIO_No_Pull;
    GPIO_InitPeripheral(GPIOB, &GPIO_InitStructure);

    /* Set the GPIO pin to low */
    GPIO_SetBits(GPIOB, GPIO_PIN_9);

    GPIO_InitStructure.Pin       = GPIO_PIN_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Input;
    GPIO_InitStructure.GPIO_Pull = GPIO_No_Pull;
    GPIO_InitPeripheral(GPIOB, &GPIO_InitStructure);
    GPIO_ConfigEXTILine(GPIOB_PORT_SOURCE, GPIO_PIN_SOURCE1);

    /*Configure the GPIO pin as input floating*/
    GPIO_InitStructure.Pin = GPIO_PIN_13;
    GPIO_InitPeripheral(GPIOC, &GPIO_InitStructure);
    GPIO_ConfigEXTILine(GPIOC_PORT_SOURCE, GPIO_PIN_SOURCE13);

    /*Configure key EXTI line*/
    EXTI_InitStructure.EXTI_Line    = EXTI_LINE1;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStructure);

    /*Configure key EXTI line*/
    EXTI_InitStructure.EXTI_Line = EXTI_LINE13;
    EXTI_InitPeripheral(&EXTI_InitStructure);

    wakeup_handler = h;
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

static pvd_handle_func pvd_handler;

void pvd_init(pvd_handle_func h)
{
    EXTI_InitType EXTI_InitStructure;
    EXTI_InitStruct(&EXTI_InitStructure);
    /* Configure EXTI Line16(PVD Output) to generate an interrupt on rising and
       falling edges */
    EXTI_ClrITPendBit(EXTI_LINE16);
    EXTI_InitStructure.EXTI_Line    = EXTI_LINE16;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStructure);
    PWR_PVDLevelConfig(PWR_PVDLEVEL_2V25);
    PWR_PvdEnable(ENABLE);
    NVIC_EnableIRQ(PVD_IRQn);
}

void PVD_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_LINE16) != RESET) {
        if (pvd_handler != NULL) {
            pvd_handler(); // Call the PVD handler with a dummy value
        }
        EXTI_ClrITPendBit(EXTI_LINE16);
    }
}
