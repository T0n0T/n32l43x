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
        .exit_Line    = EXTI_LINE0,
        .exit_irq     = EXTI0_IRQn,
    },
    {
        .port         = GPIOC,
        .clk          = RCC_APB2_PERIPH_GPIOC,
        .pin          = GPIO_PIN_1,
        .active_level = Bit_RESET,
        .exit_Line    = EXTI_LINE1,
        .exit_irq     = EXTI1_IRQn,
    },
    {
        .port         = GPIOC,
        .clk          = RCC_APB2_PERIPH_GPIOC,
        .pin          = GPIO_PIN_2,
        .active_level = Bit_RESET,
        .exit_Line    = EXTI_LINE2,
        .exit_irq     = EXTI2_IRQn,
    },
    {
        .port         = GPIOC,
        .clk          = RCC_APB2_PERIPH_GPIOC,
        .pin          = GPIO_PIN_3,
        .active_level = Bit_RESET,
        .exit_Line    = EXTI_LINE3,
        .exit_irq     = EXTI3_IRQn,
    },
    {
        .port         = GPIOA,
        .clk          = RCC_APB2_PERIPH_GPIOA,
        .pin          = GPIO_PIN_1,
        .active_level = Bit_RESET,
        .exit_Line    = EXTI_LINE1,
        .exit_irq     = EXTI1_IRQn,
    },
    {
        .port         = GPIOA,
        .clk          = RCC_APB2_PERIPH_GPIOA,
        .pin          = GPIO_PIN_2,
        .active_level = Bit_RESET,
        .exit_Line    = EXTI_LINE2,
        .exit_irq     = EXTI2_IRQn,
    },
};

void EXTI0_IRQHandler(void)
{
    if (RESET != EXTI_GetITStatus(EXTI_LINE0)) {
        EXTI_ClrITPendBit(EXTI_LINE0);
        printf("EXTI0 Happened\r\n");
    }
}

void EXTI1_IRQHandler(void)
{
    if (RESET != EXTI_GetITStatus(EXTI_LINE1)) {
        EXTI_ClrITPendBit(EXTI_LINE1);
        printf("EXTI1 Happened\r\n");
    }
}

void EXTI2_IRQHandler(void)
{
    if (RESET != EXTI_GetITStatus(EXTI_LINE2)) {
        EXTI_ClrITPendBit(EXTI_LINE2);
        printf("EXTI2 Happened\r\n");
    }
}

void EXTI3_IRQHandler(void)
{
    if (RESET != EXTI_GetITStatus(EXTI_LINE3)) {
        EXTI_ClrITPendBit(EXTI_LINE3);
        printf("EXTI3 Happened\r\n");
    }
}

void hall_registor_irq(hall_index_t index)
{
    EXTI_InitType EXTI_InitStructure;
    NVIC_InitType NVIC_InitStructure;

    GPIO_ConfigEXTILine(GET_GPIO_PORT_SOURCE(halls[index].port), GET_GPIO_PIN_SOURCE(halls[index].pin));

    /*Configure key EXTI line*/
    EXTI_InitStructure.EXTI_Line    = halls[index].exit_Line;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStructure);

    /*Set key input interrupt priority*/
    NVIC_InitStructure.NVIC_IRQChannel                   = halls[index].exit_irq;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void hall_init(void)
{
    GPIO_InitType GPIO_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);
    for (size_t i = 0; i < sizeof(halls) / sizeof(hall_t); i++) {
        RCC_EnableAPB2PeriphClk(halls[i].clk | RCC_APB2_PERIPH_AFIO, ENABLE);
        GPIO_InitStructure.Pin       = halls[i].pin;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Input; // 配置为上拉输入模式
        GPIO_InitPeripheral(halls[i].port, &GPIO_InitStructure);
    }
    RCC_EnableAPB2PeriphClk(GPIO_CLK_HALL_CTR, ENABLE);
    GPIO_InitStructure.Pin       = GPIO_PIN_HALL_CTR;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 配置为上拉输入模式
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
