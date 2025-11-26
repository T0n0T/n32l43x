#include "qpc.h"
#include "valve.h"
#include "bsp.h"
#include "at_lora.h"
#include "log.h"
#include <string.h>

#define AT                  LORAWAN
#define USART_AT            USART3
#define USART_AT_IRQn       USART3_IRQn
#define USART_AT_IRQHandler USART3_IRQHandler
#define USART_AT_DMA_TX     DMA_CH7
#define USART_AT_DMA_TX_MAP DMA_REMAP_USART3_TX
#define USART_AT_DMA_RX     DMA_CH8
#define USART_AT_DMA_RX_MAP DMA_REMAP_USART3_RX
#define AT_BUF_LEN          64U

bool                     at_module_already_on;
static const char*       AT_START_STRING = "start\r\n";
static uint8_t           _at_buf_rx[AT_BUF_LEN];
static uint8_t           _at_buf_tx[AT_BUF_LEN * 2];
static uint8_t           _at_start_match_pos;
static bool              _at_start_string_received;
static volatile uint32_t _at_len;              // Change to uint16_t for length
static char              send_expr_buffer[32]; // 最大长度为 238

static QEvt _at_evt;

static at_t at_lora;
static int  at_join_callback(char* resp);

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
                        break;                         // Found "Start", no need to check further in this packet
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
                at_fsm_copy_buffer(&at_lora, _at_buf_rx, _at_len);
            }
        }

        DMA_SetCurrDataCounter(USART_AT_DMA_RX, AT_BUF_LEN); // Reset DMA buffer size
        DMA_EnableChannel(USART_AT_DMA_RX, ENABLE);          // Re-enable DMA
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

static void at_transfer_layer_init(void)
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

    if (RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_SFTRSTF) == SET && at_module_already_on) {
        _at_start_string_received = true;
    }
}

static void at_transfer_layer_deinit(void)
{
    at_module_already_on      = false;
    _at_start_string_received = false;
    _at_start_match_pos       = 0;
    DMA_EnableChannel(USART_AT_DMA_RX, DISABLE);
    DMA_DeInit(USART_AT_DMA_RX);
    DMA_DeInit(USART_AT_DMA_TX);
    NVIC_DisableIRQ(USART_AT_IRQn);
    uart_deinit(AT);

    AT_PWR_LOW;
}

static void at_transfer_layer_transmit(const uint8_t* data, uint16_t len)
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

static int at_join_callback(char* resp)
{
    int result;
    if (sscanf(resp, "+JON: %d OK", &result) == 1) {
        return 0; // Success
    }
    return -1; // Failure
}

void at_lorawan_init(void)
{
    at_lora.transfer_init     = at_transfer_layer_init;
    at_lora.transfer_deinit   = at_transfer_layer_deinit;
    at_lora.transfer_transmit = at_transfer_layer_transmit;

    at_fsm_init(&at_lora);
}

void at_lorawan_deinit(void)
{
    at_fsm_deinit(&at_lora);
}

bool at_lorawan_is_ready(void)
{
    return _at_start_string_received;
}

at_process_result_t at_lorawan_poll(uint32_t tick)
{
    return at_fsm_request_process(&at_lora, tick);
}

void at_lorawan_config_prepare(void)
{
    static const at_cmd_t at_lora_config_cmd[] = {
        {AT_CMD_NAME(AT_LORA_CMD_WAKE), "+++", "OK\r\n", 500, 3},
        // {AT_CMD_NAME(AT_LORA_CMD_SET_MOD), "AT+MOD=1\r\n", "OK\r\n", 500, 3},
        {AT_CMD_NAME(AT_LORA_SEND_HEX), "AT+CFM=1\r\n", "OK\r\n", 500, 3},
        {AT_CMD_NAME(AT_LORA_CMD_SET_TDR), "AT+TDR=3\r\n", "OK\r\n", 500, 3},
        {AT_CMD_NAME(AT_LORA_CMD_SET_TPW), "AT+TPW=6\r\n", "OK\r\n", 500, 3},
        {AT_CMD_NAME(AT_LORA_CMD_SET_USC), "AT+USC=470500000\r\n", "OK\r\n", 500, 3},
        {
            AT_CMD_NAME(AT_LORA_CMD_JOIN),
            "AT+RJN\r\n",
            "+JON:",
            30000,
            5,
            at_join_callback,
        },
    };
    at_fsm_request_list_set(&at_lora,
                            at_lora_config_cmd,
                            sizeof(at_lora_config_cmd) / sizeof(at_lora_config_cmd[0]));
}

void at_lorawan_send_prepare(char* payload)
{
    memset(send_expr_buffer, 0, sizeof(send_expr_buffer));
    // 构造完整的 AT 命令
    snprintf(send_expr_buffer, sizeof(send_expr_buffer), "AT+TXH=21,%s\r\n", payload);

    static at_cmd_t send_cmd[] = {
        {AT_CMD_NAME(AT_LORA_SEND_HEX), send_expr_buffer, "+ACK\r\n", 5000, 3},
    };
    // 设置 FSM
    at_fsm_request_list_set(&at_lora, send_cmd, sizeof(send_cmd) / sizeof(send_cmd[0]));
}

void at_lorawan_event_post(void)
{
    QEvt_ctor(&_at_evt, VALVE_TRANSFER_INIT_SIG);
    QACTIVE_POST(AO_ValveTransfer, &_at_evt, 0);
}