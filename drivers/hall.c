#include "hall.h"

#define GET_GPIO_PORT_SOURCE(GPIOX) (uint8_t)(((GPIOX) == GPIOA) ? GPIOA_PORT_SOURCE : ((GPIOX) == GPIOB) ? GPIOB_PORT_SOURCE \
                                                                                   : ((GPIOX) == GPIOC)   ? GPIOC_PORT_SOURCE \
                                                                                   : ((GPIOX) == GPIOD)   ? GPIOD_PORT_SOURCE \
                                                                                                          : 0xF0)

#define GET_GPIO_PIN_SOURCE(GPIO_PIN) (uint8_t)(((GPIO_PIN) == GPIO_PIN_0) ? GPIO_PIN_SOURCE0 : ((GPIO_PIN) == GPIO_PIN_1) ? GPIO_PIN_SOURCE1  \
                                                                                            : ((GPIO_PIN) == GPIO_PIN_2)   ? GPIO_PIN_SOURCE2  \
                                                                                            : ((GPIO_PIN) == GPIO_PIN_3)   ? GPIO_PIN_SOURCE3  \
                                                                                            : ((GPIO_PIN) == GPIO_PIN_4)   ? GPIO_PIN_SOURCE4  \
                                                                                            : ((GPIO_PIN) == GPIO_PIN_5)   ? GPIO_PIN_SOURCE5  \
                                                                                            : ((GPIO_PIN) == GPIO_PIN_6)   ? GPIO_PIN_SOURCE6  \
                                                                                            : ((GPIO_PIN) == GPIO_PIN_7)   ? GPIO_PIN_SOURCE7  \
                                                                                            : ((GPIO_PIN) == GPIO_PIN_8)   ? GPIO_PIN_SOURCE8  \
                                                                                            : ((GPIO_PIN) == GPIO_PIN_9)   ? GPIO_PIN_SOURCE9  \
                                                                                            : ((GPIO_PIN) == GPIO_PIN_10)  ? GPIO_PIN_SOURCE10 \
                                                                                            : ((GPIO_PIN) == GPIO_PIN_11)  ? GPIO_PIN_SOURCE11 \
                                                                                            : ((GPIO_PIN) == GPIO_PIN_12)  ? GPIO_PIN_SOURCE12 \
                                                                                            : ((GPIO_PIN) == GPIO_PIN_13)  ? GPIO_PIN_SOURCE13 \
                                                                                            : ((GPIO_PIN) == GPIO_PIN_14)  ? GPIO_PIN_SOURCE14 \
                                                                                            : ((GPIO_PIN) == GPIO_PIN_15)  ? GPIO_PIN_SOURCE15 \
                                                                                                                           : 0xF0)

#define GPIO_PORT_HALL_CTR GPIOA
#define GPIO_PIN_HALL_CTR  GPIO_PIN_3
#define GPIO_CLK_HALL_CTR  RCC_APB2_PERIPH_GPIOA

static hall_t halls[] = {
    {
        .port         = GPIOC,
        .clk          = RCC_APB2_PERIPH_GPIOC,
        .pin          = GPIO_PIN_0,
        .active_level = Bit_RESET,
    },
    {
        .port         = GPIOC,
        .clk          = RCC_APB2_PERIPH_GPIOC,
        .pin          = GPIO_PIN_1,
        .active_level = Bit_RESET,
    },
    {
        .port         = GPIOC,
        .clk          = RCC_APB2_PERIPH_GPIOC,
        .pin          = GPIO_PIN_2,
        .active_level = Bit_RESET,
    },
    {
        .port         = GPIOC,
        .clk          = RCC_APB2_PERIPH_GPIOC,
        .pin          = GPIO_PIN_3,
        .active_level = Bit_RESET,
    },
    {
        .port         = GPIOC,
        .clk          = RCC_APB2_PERIPH_GPIOC,
        .pin          = GPIO_PIN_4,
        .active_level = Bit_RESET,
    },
    {
        .port         = GPIOC,
        .clk          = RCC_APB2_PERIPH_GPIOC,
        .pin          = GPIO_PIN_5,
        .active_level = Bit_RESET,
    },
    {
        .port         = GPIOC,
        .clk          = RCC_APB2_PERIPH_GPIOC,
        .pin          = GPIO_PIN_6,
        .active_level = Bit_RESET,
    },
    {
        .port         = GPIOC,
        .clk          = RCC_APB2_PERIPH_GPIOC,
        .pin          = GPIO_PIN_7,
        .active_level = Bit_RESET,
    },
    {
        .port         = GPIOC,
        .clk          = RCC_APB2_PERIPH_GPIOC,
        .pin          = GPIO_PIN_8,
        .active_level = Bit_RESET,
    },
    {
        .port         = GPIOC,
        .clk          = RCC_APB2_PERIPH_GPIOC,
        .pin          = GPIO_PIN_9,
        .active_level = Bit_RESET,
    },
    {
        .port         = GPIOC,
        .clk          = RCC_APB2_PERIPH_GPIOC,
        .pin          = GPIO_PIN_11,
        .active_level = Bit_RESET,
    },
};

void hall_init(void)
{
    GPIO_InitType GPIO_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);
    for (size_t i = 0; i < sizeof(halls) / sizeof(hall_t); i++) {
        RCC_EnableAPB2PeriphClk(halls[i].clk | RCC_APB2_PERIPH_AFIO, ENABLE);
        GPIO_InitStructure.Pin       = halls[i].pin;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Input;
        GPIO_InitPeripheral(halls[i].port, &GPIO_InitStructure);
    }
    RCC_EnableAPB2PeriphClk(GPIO_CLK_HALL_CTR, ENABLE);
    GPIO_InitStructure.Pin       = GPIO_PIN_HALL_CTR;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitPeripheral(GPIO_PORT_HALL_CTR, &GPIO_InitStructure);
}

void hall_set_ctr(uint8_t state)
{
    if (state == ENABLE) {
        GPIO_SetBits(GPIO_PORT_HALL_CTR, GPIO_PIN_HALL_CTR);
    } else {
        GPIO_ResetBits(GPIO_PORT_HALL_CTR, GPIO_PIN_HALL_CTR);
    }
}

uint8_t hall_read(hall_index_t index)
{
    uint8_t state = GPIO_ReadInputDataBit(halls[index].port, halls[index].pin);
    return (state == halls[index].active_level);
}
