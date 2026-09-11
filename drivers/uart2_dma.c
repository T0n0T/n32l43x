#include "uart2_dma.h"
#include "uart.h"

static _Alignas(uint32_t) uint8_t rx_buf[UART2_DMA_RX_SIZE];
static uart2_dma_rx_handler_t rx_handler;
static uint32_t               current_baudrate;

static void uart2_dma_restart_rx(void)
{
    DMA_EnableChannel(DMA_CH6, DISABLE);
    (void)USART2->STS;
    (void)USART2->DAT;
    DMA_ClearFlag(DMA_FLAG_GL6, DMA);
    DMA_CH6->MADDR = (uint32_t)rx_buf;
    DMA_SetCurrDataCounter(DMA_CH6, UART2_DMA_RX_SIZE);
    DMA_EnableChannel(DMA_CH6, ENABLE);
}

bool uart2_dma_set_baudrate(uint32_t baudrate)
{
    if (current_baudrate == 0 || !IS_USART_BAUDRATE(baudrate)) {
        return false;
    }

    while (USART_GetFlagStatus(USART2, USART_FLAG_TXC) == RESET);
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    USART_EnableDMA(USART2, USART_DMAREQ_RX, DISABLE);
    DMA_EnableChannel(DMA_CH6, DISABLE);
    USART_Enable(USART2, DISABLE);

    USART_InitType config;
    USART_StructInit(&config);
    config.BaudRate = baudrate;
    USART_Init(USART2, &config);
    current_baudrate = baudrate;
    uart2_dma_restart_rx();
    NVIC_ClearPendingIRQ(USART2_IRQn);
    USART_EnableDMA(USART2, USART_DMAREQ_RX, ENABLE);
    USART_Enable(USART2, ENABLE);
    __set_PRIMASK(primask);
    return true;
}

uint32_t uart2_dma_get_baudrate(void)
{
    return current_baudrate;
}

void uart2_dma_set_rx_handler(uart2_dma_rx_handler_t handler)
{
    if (current_baudrate == 0) {
        return;
    }
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    rx_handler = handler;
    uart2_dma_restart_rx();
    NVIC_ClearPendingIRQ(USART2_IRQn);
    __set_PRIMASK(primask);
}

void uart2_dma_write(const uint8_t* data, uint16_t len)
{
    if (current_baudrate == 0 || data == NULL || len == 0) {
        return;
    }
    for (uint16_t i = 0; i < len; i++) {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXDE) == RESET);
        USART_SendData(USART2, data[i]);
    }
    /* The final stop bit must leave the wire before a baud change or handoff. */
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXC) == RESET);
}

void uart2_dma_init(uint32_t baudrate, uart2_dma_rx_handler_t handler)
{
    if (!IS_USART_BAUDRATE(baudrate)) {
        return;
    }
    NVIC_DisableIRQ(USART2_IRQn);
    uart_init(BLE_SERIAL);
    current_baudrate = 115200U;
    USART_Enable(USART2, DISABLE);
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA, ENABLE);

    DMA_InitType config;
    DMA_DeInit(DMA_CH6);
    DMA_StructInit(&config);
    config.PeriphAddr    = (uint32_t)&USART2->DAT;
    config.MemAddr       = (uint32_t)rx_buf;
    config.Direction     = DMA_DIR_PERIPH_SRC;
    config.BufSize       = UART2_DMA_RX_SIZE;
    config.DMA_MemoryInc = DMA_MEM_INC_ENABLE;
    config.Priority      = DMA_PRIORITY_VERY_HIGH;
    DMA_Init(DMA_CH6, &config);
    DMA_RequestRemap(DMA_REMAP_USART2_RX, DMA, DMA_CH6, ENABLE);
    rx_handler = handler;
    uart2_dma_set_baudrate(baudrate);
    USART_ConfigInt(USART2, USART_INT_IDLEF, ENABLE);
    NVIC_SetPriority(USART2_IRQn, 0U);
    NVIC_EnableIRQ(USART2_IRQn);
}

void uart2_dma_deinit(void)
{
    if (current_baudrate == 0) {
        return;
    }
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXC) == RESET);
    NVIC_DisableIRQ(USART2_IRQn);
    USART_ConfigInt(USART2, USART_INT_IDLEF, DISABLE);
    USART_EnableDMA(USART2, USART_DMAREQ_RX, DISABLE);
    DMA_EnableChannel(DMA_CH6, DISABLE);
    USART_Enable(USART2, DISABLE);
    (void)USART2->STS;
    (void)USART2->DAT;
    DMA_ClearFlag(DMA_FLAG_GL6, DMA);
    NVIC_ClearPendingIRQ(USART2_IRQn);
    rx_handler       = NULL;
    current_baudrate = 0;
}

void USART2_IRQHandler(void)
{
    uint32_t status   = USART2->STS;
    bool     rx_error = (status & (USART_STS_OREF | USART_STS_NEF |
                                   USART_STS_PEF | USART_STS_FEF)) != 0;
    if ((status & USART_STS_IDLEF) != 0) {
        DMA_EnableChannel(DMA_CH6, DISABLE);
        (void)USART2->DAT;
        uint16_t len = UART2_DMA_RX_SIZE - DMA_GetCurrDataCounter(DMA_CH6);
        if (rx_handler != NULL) {
            rx_handler(rx_buf, len, rx_error);
        }
        uart2_dma_restart_rx();
    } else if (rx_error) {
        (void)USART2->DAT;
    }
}
