#include <board.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <stdio.h>
#include "drv_gpio.h"

void wakup_pin(void)
{
    GPIO_InitType GPIO_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA, ENABLE);
    GPIO_InitStructure.Pin       = GPIO_PIN_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Input;
    GPIO_InitStructure.GPIO_Pull = GPIO_Pull_Down;
    GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);    
    PWR_WakeUpPinEnable(WAKEUP_PIN1, ENABLE);
}

void flag_led(void)
{
    GPIO_InitType GPIO_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA, ENABLE);
    GPIO_InitStructure.Pin       = GPIO_PIN_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pull = GPIO_No_Pull;
    GPIO_InitPeripheral(GPIOA, &GPIO_InitStructure);
    GPIO_WriteBit(GPIOA, GPIO_PIN_8, Bit_SET);
}

void standby_with_wakup1(void)
{
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);
    wakup_pin();
    // flag_led();
    if (PWR_GetFlagStatus(1, PWR_STBY_FLAG) != RESET) {
        /* Clear Wake Up flag */
        PWR_ClearFlag(PWR_STBY_FLAG);
    }
    /* Check if the Wake-Up flag is set */
    if (PWR_GetFlagStatus(1, PWR_WKUP1_FLAG) != RESET) {
        /* Clear Wake Up flag */
        PWR_ClearFlag(PWR_WKUP1_FLAG);
    }
    PWR_EnterSTANDBYMode(PWR_STOPENTRY_WFI, PWR_CTRL3_RAM2RET);
}
MSH_CMD_EXPORT_ALIAS(standby_with_wakup1, test_standby, test);

void stop2_with_lptimer(void)
{
    rt_device_t _lptimer = rt_device_find("lptimer");
    rt_hwtimer_t* timer = (rt_hwtimer_t*)_lptimer;
    rt_uint32_t   cnt   = 65535;

    /* Start timer */
    timer->ops->init(timer, 1);
    timer->ops->start(timer, cnt, HWTIMER_MODE_ONESHOT);
    PWR_EnterSTOP2Mode(PWR_STOPENTRY_WFI, PWR_CTRL3_RAM1RET | PWR_CTRL3_RAM2RET);

    set_sysclock_to_pll(SystemCoreClock, SYSCLK_PLLSRC_HSE_PLLDIV2);

    // timer->ops->stop(timer);
}
MSH_CMD_EXPORT_ALIAS(stop2_with_lptimer, test_stop2, test);

