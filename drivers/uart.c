#include "uart.h"

static uart_t uarts[] = {
    {
        .tx_port     = GPIOA,
        .rx_port     = GPIOA,
        .tx_pin      = GPIO_PIN_9,
        .rx_pin      = GPIO_PIN_10,
        .tx_gpio_clk = RCC_APB2_PERIPH_GPIOA,
        .rx_gpio_clk = RCC_APB2_PERIPH_GPIOA,
        .tx_af       = GPIO_AF4_USART1,
        .rx_af       = GPIO_AF4_USART1,
        .handle      = USART1,
        .irqn        = USART1_IRQn,
        .clk_src     = RCC_APB2_PERIPH_USART1,
        .baudrate    = 115200,
        .parity      = USART_PE_NO,
        .stop_bits   = USART_STPB_1,
    },
    // {
    //     .tx_port     = GPIOB,
    //     .rx_port     = GPIOB,
    //     .tx_pin      = GPIO_PIN_4,
    //     .rx_pin      = GPIO_PIN_5,
    //     .tx_gpio_clk = RCC_APB2_PERIPH_GPIOB,
    //     .rx_gpio_clk = RCC_APB2_PERIPH_GPIOB,
    //     .tx_af       = GPIO_AF4_USART2,
    //     .rx_af       = GPIO_AF6_USART2,
    //     .handle      = USART2,
    //     .irqn        = USART2_IRQn,
    //     .clk_src     = RCC_APB1_PERIPH_USART2,
    //     .baudrate    = 115200,
    //     .parity      = USART_PE_NO,
    //     .stop_bits   = USART_STPB_1,
    // },
    // {
    //     .tx_port     = GPIOC,
    //     .rx_port     = GPIOD,
    //     .tx_pin      = GPIO_PIN_12,
    //     .rx_pin      = GPIO_PIN_2,
    //     .tx_gpio_clk = RCC_APB2_PERIPH_GPIOC,
    //     .rx_gpio_clk = RCC_APB2_PERIPH_GPIOD,
    //     .tx_af       = GPIO_AF6_UART5,
    //     .rx_af       = GPIO_AF6_UART5,
    //     .handle      = UART5,
    //     .irqn        = UART5_IRQn,
    //     .clk_src     = RCC_APB2_PERIPH_UART5,
    //     .baudrate    = 115200,
    //     .parity      = USART_PE_NO,
    //     .stop_bits   = USART_STPB_1,
    // },
};

void uart_init(void)
{
    USART_InitType USART_InitStructure;
    GPIO_InitType  GPIO_InitStructure;

    for (size_t i = 0; i < sizeof(uarts) / sizeof(uart_t); i++) {
        RCC_EnableAPB2PeriphClk(uarts[i].tx_gpio_clk | uarts[i].rx_gpio_clk, ENABLE);
        GPIO_InitStruct(&GPIO_InitStructure);

        /* Configure USARTx Tx as alternate function push-pull */
        GPIO_InitStructure.Pin            = uarts[i].tx_pin;
        GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
        GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
        GPIO_InitStructure.GPIO_Alternate = uarts[i].tx_af;
        GPIO_InitPeripheral(uarts[i].tx_port, &GPIO_InitStructure);

        /* Configure USARTx Rx as alternate function push-pull and pull-up */
        GPIO_InitStructure.Pin            = uarts[i].rx_pin;
        GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
        GPIO_InitStructure.GPIO_Alternate = uarts[i].rx_af;
        GPIO_InitPeripheral(uarts[i].rx_port, &GPIO_InitStructure);

        if (IS_RCC_APB2_PERIPH(uarts[i].clk_src)) {
            RCC_EnableAPB2PeriphClk(uarts[i].clk_src, ENABLE);
        } else {
            RCC_EnableAPB1PeriphClk(uarts[i].clk_src, ENABLE);
        }

        USART_StructInit(&USART_InitStructure);
        USART_InitStructure.BaudRate            = uarts[i].baudrate;
        USART_InitStructure.WordLength          = USART_WL_8B;
        USART_InitStructure.StopBits            = uarts[i].stop_bits;
        USART_InitStructure.Parity              = uarts[i].parity;
        USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
        USART_InitStructure.Mode                = USART_MODE_RX | USART_MODE_TX;

        /* Configure USARTx */
        USART_Init(uarts[i].handle, &USART_InitStructure);
        /* Enable the USARTx */
        USART_Enable(uarts[i].handle, ENABLE);
    }
}

#include <stdio.h>

#ifdef __GNUC__
#ifdef TINY_STDIO

static int __fputc(char c, FILE* file);

static FILE __stdio_out = FDEV_SETUP_STREAM(__fputc, NULL, NULL, _FDEV_SETUP_WRITE);

#ifdef __strong_reference
#define STDIO_ALIAS(x) __strong_reference(stdout, x);
#else
#define STDIO_ALIAS(x) FILE* const x = &__stdio_out;
#endif

FILE* const stdout = &__stdio_out;
STDIO_ALIAS(stderr);

static int __fputc(char ch, FILE* file)
{
    USART_SendData(uarts[CONSOLE].handle, (uint8_t)ch);
    while (USART_GetFlagStatus(uarts[CONSOLE].handle, USART_FLAG_TXDE) == RESET);
    return (ch);
}

#endif
#endif