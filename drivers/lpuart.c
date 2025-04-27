#include "lpuart.h"

static lpuart_t _lpuart = {
    .tx_port     = GPIOC,
    .rx_port     = GPIOC,
    .tx_pin      = GPIO_PIN_11,
    .rx_pin      = GPIO_PIN_10,
    .tx_gpio_clk = RCC_APB2_PERIPH_GPIOC,
    .rx_gpio_clk = RCC_APB2_PERIPH_GPIOC,
    .tx_af       = GPIO_AF0_LPUART,
    .rx_af       = GPIO_AF0_LPUART,
    .clk_src     = RCC_LPUARTCLK_SRC_LSE,
};

void lpuart_init(void)
{
    LPUART_InitType LPUART_InitStructure;
    GPIO_InitType   GPIO_InitStructure;

    switch (_lpuart.clk_src) {
        case RCC_LPUARTCLK_SRC_LSE: {
            RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);
            PWR->CTRL1 |= PWR_CTRL1_DRBP;
            /* Configures the External Low Speed oscillator (LSE) */
            RCC_ConfigLse(RCC_LSE_ENABLE, 0x28);
            while (RCC_GetFlagStatus(RCC_LDCTRL_FLAG_LSERD) == RESET) {
            }
            /* Specifies the LPUART clock source, LSE selected as LPUART clock */
            RCC_ConfigLPUARTClk(RCC_LPUARTCLK_SRC_LSE);
        } break;
        case RCC_LPUARTCLK_SRC_HSI: {
            /* Configures the High Speed Internal RC clock (HSI) */
            RCC_ConfigHsi(RCC_HSI_ENABLE);
            while (RCC_GetFlagStatus(RCC_CTRL_FLAG_HSIRDF) == RESET) {
            }
            /* Specifies the LPUART clock source, HSI selected as LPUART clock */
            RCC_ConfigLPUARTClk(RCC_LPUARTCLK_SRC_HSI);
        } break;
        case RCC_LPUARTCLK_SRC_SYSCLK: {
            /* Specifies the LPUART clock source, SYSCLK selected as LPUART clock */
            RCC_ConfigLPUARTClk(RCC_LPUARTCLK_SRC_SYSCLK);
        } break;
        default: {
            /* Specifies the LPUART clock source, APB1 selected as LPUART clock */
            RCC_ConfigLPUARTClk(RCC_LPUARTCLK_SRC_APB1);
        } break;
    }
    RCC_EnableRETPeriphClk(RCC_RET_PERIPH_LPUART, ENABLE);
    RCC_EnableAPB2PeriphClk(_lpuart.rx_gpio_clk | _lpuart.tx_gpio_clk, ENABLE);

    GPIO_InitStruct(&GPIO_InitStructure);

    /* connect port to USARTx_Tx */
    GPIO_InitStructure.Pin            = _lpuart.tx_pin;
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Alternate = _lpuart.tx_af;
    GPIO_InitPeripheral(_lpuart.tx_port, &GPIO_InitStructure);

    /* connect port to USARTx_Rx */
    GPIO_InitStructure.Pin            = _lpuart.rx_pin;
    GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
    GPIO_InitStructure.GPIO_Alternate = _lpuart.rx_af;
    GPIO_InitPeripheral(_lpuart.rx_port, &GPIO_InitStructure);

    NVIC_SetPriority(LPUART_WKUP_IRQn, 1);
    NVIC_EnableIRQ(LPUART_WKUP_IRQn);

    EXTI_InitType EXTI_InitStructure;
    EXTI_InitStruct(&EXTI_InitStructure);
    EXTI_InitStructure.EXTI_Line    = EXTI_LINE23;
    EXTI_InitStructure.EXTI_Mode    = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitPeripheral(&EXTI_InitStructure);

    LPUART_DeInit();
    LPUART_StructInit(&LPUART_InitStructure);

    LPUART_InitStructure.BaudRate            = 9600;
    LPUART_InitStructure.Parity              = LPUART_PE_NO;
    LPUART_InitStructure.RtsThreshold        = LPUART_RTSTH_FIFOFU;
    LPUART_InitStructure.HardwareFlowControl = LPUART_HFCTRL_NONE;
    LPUART_InitStructure.Mode                = LPUART_MODE_RX | LPUART_MODE_TX;

    /* Configure LPUART */
    LPUART_Init(&LPUART_InitStructure);
    LPUART_ConfigWakeUpMethod(LPUART_WUSTP_STARTBIT);
    LPUART_ConfigInt(LPUART_INT_WUF, ENABLE);
    LPUART_EnableWakeUpStop(ENABLE);

    NVIC_SetPriority(LPUART_IRQn, 0);
    NVIC_EnableIRQ(LPUART_IRQn);
    LPUART_ConfigInt(LPUART_INT_FIFO_NE, ENABLE);
}

void LPUART_IRQHandler(void)
{
    if (LPUART_GetIntStatus(LPUART_INT_TXC) != RESET) {
    }

    if (LPUART_GetIntStatus(LPUART_INT_WUF) != RESET) {
        LPUART_ClrIntPendingBit(LPUART_INT_WUF);
    }
}

void LPUART_WKUP_IRQHandler(void)
{
    EXTI_ClrITPendBit(EXTI_LINE23);
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

static int __fputc(char c, FILE* file)
{
    LPUART_SendData(c);
    while (LPUART_GetFlagStatus(LPUART_FLAG_TXC) == RESET);
    return c;
}

#endif
#endif