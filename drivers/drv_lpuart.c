#include <drv_lpuart.h>

#ifdef RT_USING_SERIAL
#if defined(BSP_USING_LPUART)
#include <rtdevice.h>

struct n32_lpuart {
    uint32_t                 clk_src;
    uint32_t                 gpio_clk;
    GPIO_Module*             tx_port;
    uint16_t                 tx_pin;
    GPIO_Module*             rx_port;
    uint16_t                 rx_pin;
    GPIO_Module*             cts_port;
    uint16_t                 cts_pin;
    GPIO_Module*             rts_port;
    uint16_t                 rts_pin;
    struct rt_serial_device* serial;
    char*                    device_name;
};

static void uart_isr(struct rt_serial_device* serial);

struct rt_serial_device serial0;

static struct n32_lpuart _lpuart = {
    RCC_LPUARTCLK_SRC_LSE,
    RCC_APB2_PERIPH_GPIOB, // gpio clk
    GPIOA,
    GPIO_PIN_9, // tx port, tx alternate, tx pin
    GPIOA,
    GPIO_PIN_10, // rx port, rx alternate, rx pin
    GPIOA,
    GPIO_PIN_11, // cts port, cts alternate, cts pin
    GPIOA,
    GPIO_PIN_12, // rts port, rts alternate, rts pin
    &serial0,
    "usart0",
};

void LPUART_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    uart_isr(&serial0);

    /* leave interrupt */
    rt_interrupt_leave();
}

static rt_err_t n32_lpuart_configure(struct rt_serial_device* serial, struct serial_configure* cfg)
{
    struct n32_lpuart* lpuart;
    LPUART_InitType    LPUART_InitStructure;
    GPIO_InitType      GPIO_InitStructure;

    RT_ASSERT(serial != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    lpuart = (struct n32_lpuart*)serial->parent.user_data;

    switch (lpuart->clk_src) {
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
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOB, ENABLE);

    GPIO_InitStruct(&GPIO_InitStructure);

    /* connect port to USARTx_Tx */
    GPIO_InitStructure.Pin            = lpuart->tx_pin;
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF4_LPUART;
    GPIO_InitPeripheral(lpuart->tx_port, &GPIO_InitStructure);

    /* connect port to USARTx_Rx */
    GPIO_InitStructure.Pin            = lpuart->rx_pin;
    GPIO_InitStructure.GPIO_Alternate = GPIO_AF4_LPUART;
    GPIO_InitPeripheral(lpuart->rx_port, &GPIO_InitStructure);

    LPUART_DeInit();
    LPUART_StructInit(&LPUART_InitStructure);

    LPUART_InitStructure.BaudRate = cfg->baud_rate;

    switch (cfg->parity) {
        case PARITY_ODD:
            LPUART_InitStructure.Parity = USART_PE_ODD;
            break;
        case PARITY_EVEN:
            LPUART_InitStructure.Parity = USART_PE_EVEN;
            break;
        case PARITY_NONE:
            LPUART_InitStructure.Parity = USART_PE_NO;
            break;
        default:
            break;
    }

    switch (cfg->flowcontrol) {
        case RT_SERIAL_FLOWCONTROL_NONE:
            LPUART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
            break;

        case RT_SERIAL_FLOWCONTROL_CTSRTS:
            LPUART_InitStructure.HardwareFlowControl = USART_HFCTRL_RTS_CTS;
            break;

        default:
            LPUART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
            break;
    }

    LPUART_InitStructure.Mode = USART_MODE_TX | USART_MODE_RX;

    /* Configure LPUART */
    LPUART_Init(&LPUART_InitStructure);
    return RT_EOK;
}

static rt_err_t n32_lpuart_control(struct rt_serial_device* serial, int cmd, void* arg)
{
    NVIC_InitType NVIC_InitStructure;

    /* Configure the NVIC Preemption Priority Bits */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
    switch (cmd) {
        case RT_DEVICE_CTRL_CLR_INT:
            /* disable rx irq */
            NVIC_InitStructure.NVIC_IRQChannel            = LPUART_IRQn;
            NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
            NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
            NVIC_InitStructure.NVIC_IRQChannelCmd         = DISABLE;
            NVIC_Init(&NVIC_InitStructure);

            /* disable interrupt */
            LPUART_ConfigInt(LPUART_FLAG_FIFO_NE, DISABLE);
            break;

        case RT_DEVICE_CTRL_SET_INT:
            /* enable rx irq */
            NVIC_InitStructure.NVIC_IRQChannel                   = LPUART_IRQn;
            NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
            NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 1;
            NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
            NVIC_Init(&NVIC_InitStructure);

            /* enable interrupt */
            LPUART_ConfigInt(LPUART_FLAG_FIFO_NE, ENABLE);
            break;

        default:
            break;
    }

    return RT_EOK;
}

static int n32_lpuart_putc(struct rt_serial_device* serial, char ch)
{
    LPUART_SendData(ch);
    while ((LPUART_GetFlagStatus(LPUART_FLAG_TXC) == RESET));

    return 1;
}

static int n32_lpuart_getc(struct rt_serial_device* serial)
{
    int ch;

    ch = -1;
    if (LPUART_GetFlagStatus(LPUART_FLAG_FIFO_NE) != RESET) {
        ch = LPUART_ReceiveData();
    }

    return ch;
}

static void uart_isr(struct rt_serial_device* serial)
{
    if (LPUART_GetIntStatus(LPUART_FLAG_FIFO_NE) != RESET &&
        LPUART_GetFlagStatus(LPUART_FLAG_FIFO_NE) != RESET) {
        rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
    }

    if (LPUART_GetIntStatus(LPUART_FLAG_TXC) != RESET &&
        LPUART_GetFlagStatus(LPUART_FLAG_TXC) != RESET) {
        /* Write one byte to the transmit data register */
        rt_hw_serial_isr(serial, RT_SERIAL_EVENT_TX_DONE);
    }
}

static const struct rt_uart_ops n32_lpuart_ops = {
    n32_lpuart_configure,
    n32_lpuart_control,
    n32_lpuart_putc,
    n32_lpuart_getc,
};

int rt_hw_lpuart_init(void)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    _lpuart.serial->ops    = &n32_lpuart_ops;
    _lpuart.serial->config = config;

    /* register UART device */
    rt_hw_serial_register(_lpuart.serial,
                          _lpuart.device_name,
                          RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX,
                          (void*)&_lpuart);

    return 0;
}
INIT_BOARD_EXPORT(rt_hw_lpuart_init);

#endif /* BSP_USING_LPUART */

#endif /* RT_USING_SERIAL */