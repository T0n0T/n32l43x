#include "board.h"
#include "uart.h"
#include "transport.h"

extern uint32_t tick_count;
extern transport_ctrl_t* uart2_ctrl; // Add this line

transport_ctrl_t*       uart1_ctrl;
static volatile uint8_t rx_buffer[256];

// Define callbacks
void uart1_rx_callback(const uint8_t* data, uint16_t len)
{
    // Process received data
    transport_send(uart2_ctrl, data, len);
    for (uint16_t i = 0; i < len; i++) {
        uart_putc(CONSOLE, data[i]); // Echo to console
    }
}

void uart1_tx_complete_callback(transport_status_t status)
{
    // Handle transmission completion
}

void uart1_error_callback(transport_status_t error)
{
    // Handle errors
}

// Define transport functions
transport_status_t uart1_tx_function(const uint8_t* data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        uart_putc(CONSOLE, data[i]); // Send remaining bytes
    }
    return TRANSPORT_OK;
}

uint32_t uart1_timestamp_function()
{
    // Return current timestamp (e.g., from systick)
    return tick_count;
}

void DMA_Channel4_IRQHandler(void)
{
    if (DMA_GetFlagStatus(DMA_FLAG_TC4, DMA) != RESET) {
        DMA_ClrIntPendingBit(DMA_INT_TXC4, DMA);
        DMA_ClearFlag(DMA_FLAG_TC4, DMA);
    }
}

void UART4_IRQHandler(void)
{
    if (USART_GetIntStatus(UART4, USART_INT_IDLEF) != RESET) {
        (void)UART4->STS;
        (void)UART4->DAT;
        DMA_EnableChannel(DMA_CH4, DISABLE); // Disable DMA to get current count
        uint16_t len = sizeof(rx_buffer) - DMA_GetCurrDataCounter(DMA_CH4);
        transport_notify_rx(uart1_ctrl, (uint8_t*)rx_buffer, len);
        DMA_SetCurrDataCounter(DMA_CH4, sizeof(rx_buffer)); // Reset DMA buffer size
        DMA_EnableChannel(DMA_CH4, ENABLE); // Re-enable DMA
    }
    if ((USART_GetFlagStatus(UART4, USART_FLAG_OREF) != RESET) ||
        (USART_GetFlagStatus(UART4, USART_FLAG_NEF) != RESET) ||
        (USART_GetFlagStatus(UART4, USART_FLAG_PEF) != RESET) ||
        (USART_GetFlagStatus(UART4, USART_FLAG_FEF) != RESET)) {
        /*Read the sts register first,and the read the DAT register to clear the all error flag*/
        (void)UART4->STS;
        (void)UART4->DAT;
        /* Under normal circumstances, all error flags will be cleared when the upper data is read and will not be executed here;
           users can add their own processing according to the actual scenario. */
    }
}

void uart1_transport_init(void)
{
    transport_cfg_t uart1_config = {
        .tx_buffer_size   = 256,
        .rx_buffer_size   = 256,
        .tx_chunk_size    = 64,
        .ack_enabled      = false,
        .ack_size         = 0,
        .ack_pattern      = NULL, // ACK pattern
        .ack_timeout      = 1000, // 1 second timeout
        .retry_count      = 3,
        .rx_cb            = uart1_rx_callback,
        .tx_complete_cb   = uart1_tx_complete_callback,
        .error_cb         = uart1_error_callback,
        .tx_fn            = uart1_tx_function,
        .validate_rx_fn   = NULL, // No validation function
        .get_timestamp_fn = uart1_timestamp_function,
        .alloc_fn         = malloc, // Use standard malloc
        .free_fn          = free,   // Use standard free
    };

    uart1_ctrl = malloc(sizeof(transport_ctrl_t));
    if (uart1_ctrl) {
        transport_init(uart1_ctrl, &uart1_config);
    }

    uart_init(CONSOLE);
    USART_Enable(UART4, DISABLE);

    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA, ENABLE);
    DMA_InitType DMA_InitStructure;
    DMA_DeInit(DMA_CH4);
    DMA_InitStructure.PeriphAddr     = ((uint32_t)UART4 + 0x04);
    DMA_InitStructure.MemAddr        = (uint32_t)rx_buffer;
    DMA_InitStructure.Direction      = DMA_DIR_PERIPH_SRC;
    DMA_InitStructure.BufSize        = sizeof(rx_buffer);
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.CircularMode   = DMA_MODE_CIRCULAR;
    DMA_InitStructure.Priority       = DMA_PRIORITY_VERY_HIGH;
    DMA_InitStructure.Mem2Mem        = DMA_M2M_DISABLE;
    DMA_Init(DMA_CH4, &DMA_InitStructure);
    DMA_ConfigInt(DMA_CH4, DMA_INT_TXC, ENABLE);
    DMA_RequestRemap(DMA_REMAP_UART4_RX, DMA, DMA_CH4, ENABLE);
    USART_EnableDMA(UART4, USART_DMAREQ_RX, ENABLE);
    DMA_EnableChannel(DMA_CH4, ENABLE);
    USART_Enable(UART4, ENABLE);
    USART_ConfigInt(UART4, USART_INT_IDLEF, ENABLE); // Enable UART IDLE interrupt

    NVIC_SetPriority(DMA_Channel4_IRQn, 0U);
    NVIC_EnableIRQ(DMA_Channel4_IRQn);
    NVIC_EnableIRQ(UART4_IRQn);
}
