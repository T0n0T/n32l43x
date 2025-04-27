#include <drv_lpuart.h>

#ifdef RT_USING_SERIAL
#if defined(BSP_USING_LPUART)
#include <rtdevice.h>

struct n32_lpuart {
    uint32_t                 clk_src;
    uint32_t                 tx_gpio_clk;
    GPIO_Module*             tx_port;
    uint32_t                 tx_af;
    uint16_t                 tx_pin;
    uint32_t                 rx_gpio_clk;
    GPIO_Module*             rx_port;
    uint32_t                 rx_af;
    uint16_t                 rx_pin;
    struct rt_serial_device* serial;
    char*                    device_name;
};

static void uart_isr(struct rt_serial_device* serial);

struct rt_serial_device serial0;

static struct n32_lpuart _lpuart = {
    RCC_LPUARTCLK_SRC_LSE,
    RCC_APB2_PERIPH_GPIOC, // gpio clk
    GPIOC,
    GPIO_AF0_LPUART,
    GPIO_PIN_10, // tx port, tx alternate, tx pin
    RCC_APB2_PERIPH_GPIOC,
    GPIOC,
    GPIO_AF0_LPUART,
    GPIO_PIN_11, // rx port, rx alternate, rx pin
    &serial0,
    "lpuart",
};

void LPUART_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    uart_isr(&serial0);

    /* leave interrupt */
    rt_interrupt_leave();
}

void LPUART_WKUP_IRQHandler(void)
{
    /* Clear EXTI Line23 Pending Bit */
    EXTI_ClrITPendBit(EXTI_LINE23);
}

static rt_err_t n32_lpuart_configure(struct rt_serial_device* serial, struct serial_configure* cfg)
{
    struct n32_lpuart* lpuart;
    LPUART_InitType    LPUART_InitStructure;
    GPIO_InitType      GPIO_InitStructure;

    RT_ASSERT(serial != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);
    RT_ASSERT(cfg->baud_rate <= 9600);

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
    RCC_EnableAPB2PeriphClk(lpuart->rx_gpio_clk | lpuart->tx_gpio_clk, ENABLE);

    GPIO_InitStruct(&GPIO_InitStructure);

    /* connect port to USARTx_Tx */
    GPIO_InitStructure.Pin            = lpuart->tx_pin;
    GPIO_InitStructure.GPIO_Mode      = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Alternate = lpuart->tx_af;
    GPIO_InitPeripheral(lpuart->tx_port, &GPIO_InitStructure);

    /* connect port to USARTx_Rx */
    GPIO_InitStructure.Pin            = lpuart->rx_pin;
    GPIO_InitStructure.GPIO_Pull      = GPIO_Pull_Up;
    GPIO_InitStructure.GPIO_Alternate = lpuart->rx_af;
    GPIO_InitPeripheral(lpuart->rx_port, &GPIO_InitStructure);

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

    LPUART_InitStructure.BaudRate = cfg->baud_rate;

    switch (cfg->parity) {
        case PARITY_ODD:
            LPUART_InitStructure.Parity = LPUART_PE_ODD;
            break;
        case PARITY_EVEN:
            LPUART_InitStructure.Parity = LPUART_PE_EVEN;
            break;
        case PARITY_NONE:
            LPUART_InitStructure.Parity = LPUART_PE_NO;
            break;
        default:
            break;
    }

    switch (cfg->flowcontrol) {
        case RT_SERIAL_FLOWCONTROL_NONE:
            LPUART_InitStructure.HardwareFlowControl = LPUART_HFCTRL_NONE;
            break;

        case RT_SERIAL_FLOWCONTROL_CTSRTS:
            LPUART_InitStructure.HardwareFlowControl = LPUART_HFCTRL_RTS_CTS;
            break;

        default:
            LPUART_InitStructure.HardwareFlowControl = LPUART_HFCTRL_NONE;
            break;
    }

    LPUART_InitStructure.RtsThreshold = LPUART_RTSTH_FIFOFU;
    LPUART_InitStructure.Mode         = LPUART_MODE_RX | LPUART_MODE_TX;
    /* Configure LPUART */
    LPUART_Init(&LPUART_InitStructure);
    LPUART_ConfigWakeUpMethod(LPUART_WUSTP_STARTBIT);
    LPUART_ConfigInt(LPUART_INT_WUF, ENABLE);
    LPUART_EnableWakeUpStop(ENABLE);
    return RT_EOK;
}

static rt_err_t n32_lpuart_control(struct rt_serial_device* serial, int cmd, void* arg)
{
    rt_ubase_t ctrl_arg = (rt_ubase_t)arg;
    switch (cmd) {
        case RT_DEVICE_CTRL_CLR_INT:
            /* disable interrupt */
            NVIC_DisableIRQ(LPUART_IRQn);
            if (ctrl_arg == RT_DEVICE_FLAG_INT_RX) {
                /* disable interrupt */
                LPUART_ConfigInt(LPUART_INT_FIFO_NE, DISABLE);
            } else if (ctrl_arg == RT_DEVICE_FLAG_INT_TX) {
                /* disable interrupt */
                LPUART_ConfigInt(LPUART_INT_TXC, DISABLE);
            }
            break;

        case RT_DEVICE_CTRL_SET_INT:
            /* enable interrupt */
            NVIC_SetPriority(LPUART_IRQn, 0);
            NVIC_EnableIRQ(LPUART_IRQn);
            if (ctrl_arg == RT_DEVICE_FLAG_INT_RX) {
                /* enable interrupt */
                LPUART_ConfigInt(LPUART_INT_FIFO_NE, ENABLE);
            } else if (ctrl_arg == RT_DEVICE_FLAG_INT_TX) {
                /* enable interrupt */
                LPUART_ConfigInt(LPUART_INT_TXC, ENABLE);
            }
            break;

        default:
            break;
    }

    return RT_EOK;
}

static int n32_lpuart_putc(struct rt_serial_device* serial, char ch)
{
    LPUART_ClrIntPendingBit(LPUART_INT_TXC);
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
    if (LPUART_GetIntStatus(LPUART_INT_FIFO_NE) != RESET) {
        rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
    }

    if (LPUART_GetIntStatus(LPUART_INT_TXC) != RESET) {
        rt_hw_serial_isr(serial, RT_SERIAL_EVENT_TX_DONE);
    }

    if (LPUART_GetIntStatus(LPUART_INT_WUF) != RESET) {
        LPUART_ClrIntPendingBit(LPUART_INT_WUF);
    }
}

// rt_err_t n32_suspend(const struct rt_device* device, rt_uint8_t mode)
// {
//     struct n32_lpuart* lpuart = (struct n32_lpuart*)device->user_data;
//     if (mode == PM_SLEEP_MODE_DEEP) {
//         LPUART_ConfigWakeUpMethod(LPUART_WUSTP_STARTBIT);
//         LPUART_ConfigInt(LPUART_INT_WUF, ENABLE);
//         LPUART_EnableWakeUpStop(ENABLE);
//     }

//     return 0;
// }

// void n32_resume(const struct rt_device* device, rt_uint8_t mode)
// {
//     LPUART_EnableWakeUpStop(DISABLE);
// }

// rt_err_t n32_frequency_change(const struct rt_device* device, rt_uint8_t mode)
// {
//     return 0;
// }

// static const struct rt_device_pm_ops n32_lpuart_pm_ops = {
//     n32_suspend,
//     n32_resume,
//     n32_frequency_change,
// };

static const struct rt_uart_ops n32_lpuart_ops = {
    n32_lpuart_configure,
    n32_lpuart_control,
    n32_lpuart_putc,
    n32_lpuart_getc,
};

int rt_hw_lpuart_init(void)
{
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    config.baud_rate               = BAUD_RATE_9600;
    _lpuart.serial->ops            = &n32_lpuart_ops;
    _lpuart.serial->config         = config;

    /* register UART device */
    rt_hw_serial_register(_lpuart.serial,
                          _lpuart.device_name,
                          RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX,
                          (void*)&_lpuart);
    return 0;
}

#endif /* BSP_USING_LPUART */

#endif /* RT_USING_SERIAL */