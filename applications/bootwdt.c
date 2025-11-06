#include "bootloader.h"

#define WDT_GUARD_PORT GPIOB
#define WDT_GUARD_CLK  RCC_APB2_PERIPH_GPIOB
#define WDT_GUARD_PIN  GPIO_PIN_9
#define WDT_GUARD_FEED WDT_GUARD_PORT->POD ^= WDT_GUARD_PIN

void bootloader_wdt_init(void)
{
    // 初始化看门狗
    GPIO_InitType GPIO_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);

    /* Enable the GPIO Clock */
    RCC_EnableAPB2PeriphClk(WDT_GUARD_CLK, ENABLE);

    /*Configure the GPIO pin as input floating*/
    GPIO_InitStructure.Pin       = WDT_GUARD_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pull = GPIO_No_Pull;
    GPIO_InitPeripheral(WDT_GUARD_PORT, &GPIO_InitStructure);
}

void bootloader_wdt_feed(void)
{
    WDT_GUARD_FEED;
}