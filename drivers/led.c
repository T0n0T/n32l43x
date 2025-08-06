#include "led.h"

static led_t leds[] = {
    {
        .port         = GPIOA,
        .clk          = RCC_APB2_PERIPH_GPIOA,
        .pin          = GPIO_PIN_12,
        .active_level = Bit_SET,
    },
    {
        .port         = GPIOA,
        .clk          = RCC_APB2_PERIPH_GPIOA,
        .pin          = GPIO_PIN_11,
        .active_level = Bit_SET,
    },
    {
        .port         = GPIOA,
        .clk          = RCC_APB2_PERIPH_GPIOA,
        .pin          = GPIO_PIN_15,
        .active_level = Bit_SET,
    }};

void led_init(void)
{
    GPIO_InitType GPIO_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);
    for (size_t i = 0; i < sizeof(leds) / sizeof(led_t); i++) {
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
