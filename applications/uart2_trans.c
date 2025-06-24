#include "board.h"
#include "uart.h"
#include "transport.h"

#define BLE_PWR_PORT GPIOB
#define BLE_PWR_CLK  RCC_APB2_PERIPH_GPIOB
#define BLE_PWR_PIN  GPIO_PIN_6
#define BLE_PWR_HIGH BLE_PWR_PORT->PBSC = BLE_PWR_PIN;
#define BLE_PWR_LOW  BLE_PWR_PORT->PBC = BLE_PWR_PIN;

extern uint32_t tick_count;
extern transport_ctrl_t* uart1_ctrl; // Add this line

transport_ctrl_t*       uart2_ctrl;
static volatile uint8_t ch;

// Define callbacks
void uart2_rx_callback(const uint8_t* data, uint16_t len)
{
    // Process received data
    transport_send(uart1_ctrl, data, len);
}

void uart2_tx_complete_callback(transport_status_t status)
{
    // Handle transmission completion
}

void uart2_error_callback(transport_status_t error)
{
    // Handle errors
}

// Define transport functions
transport_status_t uart2_tx_function(const uint8_t* data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        uart_putc(BLE_SERIAL, data[i]); // Send remaining bytes
    }
    return TRANSPORT_OK;
}

uint32_t uart2_timestamp_function()
{
    // Return current timestamp (e.g., from systick)
    return tick_count;
}

void DMA_Channel6_IRQHandler(void)
{
    if (DMA_GetFlagStatus(DMA_FLAG_TC6, DMA) != RESET) {
        // printf("%c", ch);
        transport_notify_rx(uart2_ctrl, &ch, 1);
        DMA_ClrIntPendingBit(DMA_INT_TXC6, DMA);
        DMA_ClearFlag(DMA_FLAG_TC6, DMA);
    }
}

void uart2_transport_init(void)
{
    transport_cfg_t uart2_config = {
        .tx_buffer_size   = 256,
        .rx_buffer_size   = 256,
        .tx_chunk_size    = 64,
        .ack_enabled      = false,
        .ack_size         = 0,
        .ack_pattern      = NULL, // ACK pattern
        .ack_timeout      = 1000, // 1 second timeout
        .retry_count      = 3,
        .rx_cb            = uart2_rx_callback,
        .tx_complete_cb   = uart2_tx_complete_callback,
        .error_cb         = uart2_error_callback,
        .tx_fn            = uart2_tx_function,
        .validate_rx_fn   = NULL, // No validation function
        .get_timestamp_fn = uart2_timestamp_function,
        .alloc_fn         = malloc, // Use standard malloc
        .free_fn          = free,   // Use standard free
    };

    uart2_ctrl = malloc(sizeof(transport_ctrl_t));
    if (uart2_ctrl) {
        transport_init(uart2_ctrl, &uart2_config);
    }

    GPIO_InitType GPIO_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);
    RCC_EnableAPB2PeriphClk(BLE_PWR_CLK, ENABLE);
    GPIO_InitStructure.Pin       = BLE_PWR_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitPeripheral(BLE_PWR_PORT, &GPIO_InitStructure);

    BLE_PWR_HIGH;
    
    uart_init(BLE_SERIAL);
    USART_Enable(USART2, DISABLE);

    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA, ENABLE);
    DMA_InitType DMA_InitStructure;
    DMA_DeInit(DMA_CH6);
    DMA_InitStructure.PeriphAddr     = ((uint32_t)USART2 + 0x04);
    DMA_InitStructure.MemAddr        = (uint32_t)&ch;
    DMA_InitStructure.Direction      = DMA_DIR_PERIPH_SRC;
    DMA_InitStructure.BufSize        = 1;
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.CircularMode   = DMA_MODE_CIRCULAR;
    DMA_InitStructure.Priority       = DMA_PRIORITY_VERY_HIGH;
    DMA_InitStructure.Mem2Mem        = DMA_M2M_DISABLE;
    DMA_Init(DMA_CH6, &DMA_InitStructure);
    DMA_ConfigInt(DMA_CH6, DMA_INT_TXC, ENABLE);
    DMA_RequestRemap(DMA_REMAP_USART2_RX, DMA, DMA_CH6, ENABLE);
    USART_EnableDMA(USART2, USART_DMAREQ_RX, ENABLE);
    DMA_EnableChannel(DMA_CH6, ENABLE);
    USART_Enable(USART2, ENABLE);

    NVIC_SetPriority(DMA_Channel6_IRQn, 0U);
    NVIC_EnableIRQ(DMA_Channel6_IRQn);
}
