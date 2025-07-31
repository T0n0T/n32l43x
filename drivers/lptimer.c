#include "lptimer.h"
#include <n32l43x_lptim.h>

static lptimer_irq_callback_t lptimer_callback = NULL;

static void LPTIMNVIC_Config(FunctionalState Cmd)
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

    NVIC_SetPriority(LPTIM_WKUP_IRQn, 5);
    NVIC_EnableIRQ(LPTIM_WKUP_IRQn);
}

#include "SEGGER_SYSVIEW.h"

void LPTIM_WKUP_IRQHandler(void)
{
    SEGGER_SYSVIEW_RecordEnterISR();
    if (LPTIM_IsActiveFlag_CMPM(LPTIM) != RESET) {
        if (lptimer_callback != NULL) {
            lptimer_callback();
        }
        LPTIM_ClearFLAG_CMPM(LPTIM);
        EXTI_ClrITPendBit(EXTI_LINE24);
    }
    SEGGER_SYSVIEW_RecordExitISR();
}

void lptimer_init(void) {
    /* Enable LPTIM clock as 40000Hz */
    RCC_EnableLsi(ENABLE);
    RCC_ConfigLPTIMClk(RCC_LPTIMCLK_SRC_LSI);
    RCC_EnableRETPeriphClk(RCC_RET_PERIPH_LPTIM, ENABLE);

    /* Initialize LPTIM */
    LPTIMNVIC_Config(ENABLE);
    LPTIM_SetPrescaler(LPTIM, LPTIM_PRESCALER_DIV1);
}

void lptimer_start(uint32_t cnt, lptimer_irq_callback_t cb)
{
    LPTIM_EnableIT_CMPM(LPTIM);
    LPTIM_Enable(LPTIM);

    lptimer_callback = cb;
    /* Set compare value */
    LPTIM_SetAutoReload(LPTIM, cnt);
    LPTIM_SetCompare(LPTIM, 0);
    LPTIM_StartCounter(LPTIM, LPTIM_OPERATING_MODE_CONTINUOUS);
}

void lptimer_stop(void)
{
    LPTIM_Disable(LPTIM);
    lptimer_callback = NULL;
}