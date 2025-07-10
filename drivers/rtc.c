#include "rtc.h"

void rtc_init(void)
{
    RTC_InitType RTC_InitStructure;

    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_AFIO, ENABLE);
    RCC_EnableRtcClk(DISABLE);
    PWR_BackupAccessEnable(ENABLE);

    RCC_ConfigLse(RCC_LSE_ENABLE, 0x1FF);
    while (RCC_GetFlagStatus(RCC_LDCTRL_FLAG_LSERD) == RESET);
    RCC_ConfigRtcClk(RCC_RTCCLK_SRC_LSE);
    RCC_EnableRtcClk(ENABLE);
    RTC_WaitForSynchro();

    RTC_InitStructure.RTC_AsynchPrediv = 127;
    RTC_InitStructure.RTC_SynchPrediv  = 255; //1hz
    RTC_InitStructure.RTC_HourFormat   = RTC_24HOUR_FORMAT;
    RTC_Init(&RTC_InitStructure);

    RTC_EnableWakeUp(DISABLE);
    RTC_ConfigWakeUpClock(RTC_WKUPCLK_CK_SPRE_16BITS);
    RTC_SetWakeUpCounter(1); // 1s wakeup; val=t*2-1

    EXTI_InitType EXTI_InitStructure;
    NVIC_InitType NVIC_InitStructure;
    EXTI_ClrITPendBit(EXTI_LINE20);
    EXTI_InitStruct(&EXTI_InitStructure);
    EXTI_InitStructure.EXTI_Line    = EXTI_LINE20;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStructure);
    /* Enable the RTC WakeUp Interrupt */
    NVIC_EnableIRQ(RTC_IRQn);

    /* Enable the RTC Wakeup Interrupt */
    RTC_ClrIntPendingBit(RTC_INT_WUT);
    RTC_ConfigInt(RTC_INT_WUT, ENABLE);
    RTC_EnableWakeUp(ENABLE);
}
