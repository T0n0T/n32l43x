#include "led.h"

static led_t leds[LED_MAX] = {
    {
        .port         = GPIOB,
        .clk          = RCC_APB2_PERIPH_GPIOB,
        .pin          = GPIO_PIN_4,
        .active_level = Bit_RESET,
    }

};

void led_init(void)
{
    GPIO_InitType GPIO_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);
    for (size_t i = 0; i < LED_MAX; i++)
    {
        RCC_EnableAPB2PeriphClk(leds[i].clk, ENABLE);
        GPIO_InitStructure.Pin = leds[i].pin;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        GPIO_InitPeripheral(leds[i].port, &GPIO_InitStructure);
    }   
}

void led_on(led_index_t index)
{
    GPIO_WriteBit(leds[index].port, leds[index].pin, leds[index].active_level);
}

void led_off(led_index_t index)
{
    GPIO_WriteBit(leds[index].port, leds[index].pin, !leds[index].active_level);
}

void led_toggle(led_index_t index)
{
    leds[index].port->POD ^= leds[index].pin;
}
