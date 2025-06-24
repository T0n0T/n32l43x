#include "uart.h"

static uart_t uarts[] = {
    {
        .tx_port     = GPIOC,
        .rx_port     = GPIOC,
        .tx_pin      = GPIO_PIN_10,
        .rx_pin      = GPIO_PIN_11,
        .tx_gpio_clk = RCC_APB2_PERIPH_GPIOC,
        .rx_gpio_clk = RCC_APB2_PERIPH_GPIOC,
        .tx_af       = GPIO_AF6_UART4,
        .rx_af       = GPIO_AF6_UART4,
        .handle      = UART4,
        .irqn        = UART4_IRQn,
        .clk_src     = RCC_APB2_PERIPH_UART4,
        .baudrate    = 115200,
        .parity      = USART_PE_NO,
        .stop_bits   = USART_STPB_1,
    },
    {
        .tx_port     = GPIOB,
        .rx_port     = GPIOB,
        .tx_pin      = GPIO_PIN_4,
        .rx_pin      = GPIO_PIN_5,
        .tx_gpio_clk = RCC_APB2_PERIPH_GPIOB,
        .rx_gpio_clk = RCC_APB2_PERIPH_GPIOB,
        .tx_af       = GPIO_AF4_USART2,
        .rx_af       = GPIO_AF6_USART2,
        .handle      = USART2,
        .irqn        = USART2_IRQn,
        .clk_src     = RCC_APB1_PERIPH_USART2,
        .baudrate    = 9600,
        .parity      = USART_PE_NO,
        .stop_bits   = USART_STPB_1,
    }
};

void uart_init(uart_index_t index)
{
    if (index >= UART_MAX) {
        return;
    }

    USART_InitType USART_InitStructure;
    GPIO_InitType  GPIO_InitStructure;

    RCC_EnableAPB2PeriphClk(uarts[index].tx_gpio_clk | uarts[index].rx_gpio_clk, ENABLE);
    GPIO_InitStruct(&GPIO_InitStructure);

    /* Configure USARTx Tx as alternate function push-pull */
    GPIO_InitStructure.Pin            = uarts[index].tx_pin;
    GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Alternate = uarts[index].tx_af;
    GPIO_InitPeripheral(uarts[index].tx_port, &GPIO_InitStructure);

    /* Configure USARTx Rx as alternate function push-pull and pull-up */
    GPIO_InitStructure.Pin            = uarts[index].rx_pin;
    GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
    GPIO_InitStructure.GPIO_Alternate = uarts[index].rx_af;
    GPIO_InitPeripheral(uarts[index].rx_port, &GPIO_InitStructure);

    if (uarts[index].handle == USART1 ||
        uarts[index].handle == UART4 ||
        uarts[index].handle == UART5) {
        RCC_EnableAPB2PeriphClk(uarts[index].clk_src, ENABLE);
    } else if (uarts[index].handle == USART2 ||
               uarts[index].handle == USART3) {
        RCC_EnableAPB1PeriphClk(uarts[index].clk_src, ENABLE);
    }

    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.BaudRate            = uarts[index].baudrate;
    USART_InitStructure.WordLength          = USART_WL_8B;
    USART_InitStructure.StopBits            = uarts[index].stop_bits;
    USART_InitStructure.Parity              = uarts[index].parity;
    USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
    USART_InitStructure.Mode                = USART_MODE_RX | USART_MODE_TX;

    /* Configure USARTx */
    USART_Init(uarts[index].handle, &USART_InitStructure);
    /* Enable the USARTx */
    USART_Enable(uarts[index].handle, ENABLE);
}

void uart_deinit(uart_index_t index)
{
    if (index >= UART_MAX) {
        return;
    }

    USART_Module* usart = uarts[index].handle;

    /* Disable the USARTx */
    USART_Enable(usart, DISABLE);
    /* Deinitialize the USARTx peripheral */
    USART_DeInit(usart);
}

void uart_control(uart_index_t index, uint16_t int_flag, bool state)
{
    if (index >= UART_MAX) {
        return;
    }

    USART_Module* usart = uarts[index].handle;

    if (!int_flag) {
        if (state == true) {
            USART_Enable(usart, ENABLE);
        } else {
            USART_Enable(usart, DISABLE);
        }
    } else {
        if (state == true) {
            USART_ConfigInt(usart, int_flag, ENABLE);
        } else {
            USART_ConfigInt(usart, int_flag, DISABLE);
        }
    }
}

void uart_putc(uart_index_t index, const uint8_t data)
{
    if (index >= UART_MAX) {
        return;
    }
    USART_Module* usart = uarts[index].handle;

    USART_SendData(usart, data);
    while (USART_GetFlagStatus(usart, USART_FLAG_TXC) == RESET);
}

char uart_getc(uart_index_t index)
{
    if (index >= UART_MAX) {
        return 0;
    }

    USART_Module* usart = uarts[index].handle;

    while (USART_GetFlagStatus(uarts[index].handle, USART_FLAG_RXDNE) == RESET);
    return (char)USART_ReceiveData(usart);
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
    uart_putc(CONSOLE, ch);
    return (ch);
}

#endif
#endif