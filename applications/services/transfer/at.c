#include "qpc.h" // QP/C real-time embedded framework
#include "bsp.h" // Board Support Package interface
#include "stdio.h"
#include "string.h"
#include "at.h"
#include "valve.h"
#include "log.h"

static QEvt              _at_evt;
static uint8_t           _at_buf_rx[AT_BUF_LEN];
static uint8_t           _at_buf_tx[AT_BUF_LEN * 2];
static uint8_t           _at_start_match_pos;
static bool              _at_start_string_received;
static volatile uint16_t _at_len; // Change to uint16_t for length
static const char*       AT_START_STRING = "start\r\n";
bool                     at_module_already_on;

void USART_AT_IRQHandler(void)
{
    if (USART_GetIntStatus(USART_AT, USART_INT_IDLEF) != RESET) {
        (void)USART_AT->STS;
        (void)USART_AT->DAT;
        DMA_EnableChannel(USART_AT_DMA_RX, DISABLE); // Disable DMA to get current count
        _at_len = AT_BUF_LEN - DMA_GetCurrDataCounter(USART_AT_DMA_RX);

        if (!_at_start_string_received) {
            // Check for start string in the received data
            for (uint16_t i = 0; i < _at_len; i++) {
                if (_at_buf_rx[i] == AT_START_STRING[_at_start_match_pos]) {
                    _at_start_match_pos++;
                    if (_at_start_match_pos == strlen(AT_START_STRING)) {
                        APP_LOG_DEBUG("Start AT");
                        _at_start_string_received = true;
                        _at_start_match_pos       = 0; // Reset for next potential "Start" if needed
                        break;                          // Found "Start", no need to check further in this packet
                    }
                } else {
                    _at_start_match_pos = 0; // Reset if mismatch
                }
            }
        } else {
            // Process the received command
            // Assuming command ends with '\n'
            if (_at_len > 0 && _at_buf_rx[_at_len - 1] == '\n') {
                _at_buf_rx[_at_len] = '\0'; // Null-terminate the string
                // QACTIVE_POST(AO_ValveTransfer, &_at_evt, 0U);
            }
        }

        DMA_SetCurrDataCounter(USART_AT_DMA_RX, AT_BUF_LEN); // Reset DMA buffer size
        DMA_EnableChannel(USART_AT_DMA_RX, ENABLE);           // Re-enable DMA
    }
    if ((USART_GetFlagStatus(USART_AT, USART_FLAG_OREF) != RESET) ||
        (USART_GetFlagStatus(USART_AT, USART_FLAG_NEF) != RESET) ||
        (USART_GetFlagStatus(USART_AT, USART_FLAG_PEF) != RESET) ||
        (USART_GetFlagStatus(USART_AT, USART_FLAG_FEF) != RESET)) {
        /*Read the sts register first,and the read the DAT register to clear the all error flag*/
        (void)USART_AT->STS;
        (void)USART_AT->DAT;
        /* Under normal circumstances, all error flags will be cleared when the upper data is read and will not be executed here;
           users can add their own processing according to the actual scenario. */
    }
}

void at_init(void)
{
    AT_PWR_HIGH;

    uart_init(AT);
    USART_Enable(USART_AT, DISABLE);

    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA, ENABLE);
    DMA_InitType DMA_InitStructure;
    DMA_DeInit(USART_AT_DMA_RX);
    DMA_InitStructure.PeriphAddr     = (uint32_t)&USART_AT->DAT;
    DMA_InitStructure.MemAddr        = (uint32_t)_at_buf_rx;
    DMA_InitStructure.Direction      = DMA_DIR_PERIPH_SRC;
    DMA_InitStructure.BufSize        = AT_BUF_LEN;
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.CircularMode   = DMA_MODE_CIRCULAR;
    DMA_InitStructure.Priority       = DMA_PRIORITY_VERY_HIGH;
    DMA_InitStructure.Mem2Mem        = DMA_M2M_DISABLE;
    DMA_Init(USART_AT_DMA_RX, &DMA_InitStructure);
    DMA_RequestRemap(USART_AT_DMA_RX_MAP, DMA, USART_AT_DMA_RX, ENABLE);
    USART_EnableDMA(USART_AT, USART_DMAREQ_RX, ENABLE);
    DMA_EnableChannel(USART_AT_DMA_RX, ENABLE);
    USART_Enable(USART_AT, ENABLE);
    USART_ConfigInt(USART_AT, USART_INT_IDLEF, ENABLE); // Enable USART IDLE interrupt

    NVIC_EnableIRQ(USART_AT_IRQn); // Enable USART2 interrupt

    memset(_at_buf_rx, 0, sizeof(_at_buf_rx));
    _at_len                   = 0;     // Initialize buffer length
    _at_start_match_pos       = 0;     // Initialize Start string match position
    _at_start_string_received = false; // Initialize Start string received flag

    // QEvt_ctor(&_at_evt, VALVE_AT_PASS_SIG);// here declare at pass event

    if (RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_SFTRSTF) == SET && at_module_already_on) {
        _at_start_string_received = true;
    }
}

void at_deinit(void)
{
    BLE_PWR_LOW;
    at_module_already_on      = false;
    _at_start_string_received = false;
    _at_start_match_pos       = 0;
    DMA_EnableChannel(USART_AT_DMA_RX, DISABLE);
    DMA_DeInit(USART_AT_DMA_RX);
    DMA_DeInit(USART_AT_DMA_TX);
    NVIC_DisableIRQ(USART_AT_IRQn);
    uart_deinit(BLE);
}

void at_dma_transmit(const uint8_t* data, uint16_t len)
{
    memcpy(_at_buf_tx, data, len); // Copy data to transmit buffer
    DMA_InitType DMA_InitStructure;
    DMA_DeInit(USART_AT_DMA_TX);
    DMA_InitStructure.PeriphAddr     = (uint32_t)&USART_AT->DAT;
    DMA_InitStructure.MemAddr        = (uint32_t)_at_buf_tx;
    DMA_InitStructure.Direction      = DMA_DIR_PERIPH_DST;
    DMA_InitStructure.BufSize        = len;
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.CircularMode   = DMA_MODE_NORMAL;
    DMA_InitStructure.Priority       = DMA_PRIORITY_HIGH;
    DMA_InitStructure.Mem2Mem        = DMA_M2M_DISABLE;
    DMA_Init(USART_AT_DMA_TX, &DMA_InitStructure);
    DMA_RequestRemap(USART_AT_DMA_TX_MAP, DMA, USART_AT_DMA_TX, ENABLE);
    USART_EnableDMA(USART_AT, USART_DMAREQ_TX, ENABLE);
    DMA_EnableChannel(USART_AT_DMA_TX, ENABLE);
}