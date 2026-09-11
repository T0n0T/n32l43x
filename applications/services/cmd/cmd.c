#include "qpc.h" // QP/C real-time embedded framework
#include "bsp.h" // Board Support Package interface
#include "stdio.h"
#include "string.h"
#include "cmd.h"
#include "valve.h"
#include "log.h"

typedef struct {
    ValveEvt evt;
    char     buf[CMD_BUF_LEN];
} cmd_rx_slot_t;

typedef struct {
    uint16_t len;
    uint8_t  buf[CMD_TX_BUF_LEN];
} cmd_tx_slot_t;

static QEvt              _cmd_evt_prepare;
static uint8_t           _cmd_buf_rx[CMD_BUF_LEN];
static cmd_rx_slot_t     _cmd_rx_slot;
static cmd_tx_slot_t     _cmd_tx_queue[CMD_TX_QUEUE_SLOTS];
static volatile uint8_t  _cmd_tx_head;
static volatile uint8_t  _cmd_tx_tail;
static volatile bool     _cmd_tx_active;
static volatile bool     _cmd_command_active;
static volatile bool     _cmd_async_pending;
static volatile bool     _cmd_execution_done;
static uint8_t           _cmd_start_match_pos;
static bool              _cmd_start_string_received;
static bool              _cmd_set_name;
static const command_t   commands[]       = CMD_DEFINE_LIST;
static const char*       CMD_START_STRING = "Start";
bool                     cmd_module_already_on;

static uint8_t cmd_tx_next_index(uint8_t index)
{
    index++;
    return (index == CMD_TX_QUEUE_SLOTS) ? 0U : index;
}

static bool cmd_post_received(const uint8_t* data, uint16_t len)
{
    if (len == 0U || len >= CMD_BUF_LEN) {
        return false;
    }

    QF_CRIT_ENTRY();
    if (_cmd_command_active) {
        QF_CRIT_EXIT();
        return false;
    }

    _cmd_command_active = true;
    _cmd_async_pending   = false;
    _cmd_execution_done  = false;

    memcpy(_cmd_rx_slot.buf, data, len);
    _cmd_rx_slot.buf[len] = '\0';
    QEvt_ctor(&_cmd_rx_slot.evt.super, VALVE_CMD_PARSE_SIG);
    _cmd_rx_slot.evt.msg     = _cmd_rx_slot.buf;
    _cmd_rx_slot.evt.evtType = VALVE_CMD;
    QF_CRIT_EXIT();

    bool const posted = QACTIVE_POST_X(AO_ValveConf, &_cmd_rx_slot.evt.super, 1U, 0U);
    if (!posted) {
        QF_CRIT_ENTRY();
        _cmd_command_active = false;
        QF_CRIT_EXIT();
    }
    return posted;
}

static void cmd_rx_release(char* input)
{
    QF_CRIT_ENTRY();
    if (input == _cmd_rx_slot.buf) {
        _cmd_rx_slot.evt.msg = NULL;
    }
    QF_CRIT_EXIT();
}

static void cmd_try_complete(void)
{
    QF_CRIT_ENTRY();
    if (_cmd_command_active && _cmd_execution_done && !_cmd_async_pending &&
        !_cmd_tx_active && _cmd_tx_head == _cmd_tx_tail) {
        _cmd_command_active = false;
        _cmd_execution_done  = false;
    }
    QF_CRIT_EXIT();
}

void cmd_async_begin(void)
{
    QF_CRIT_ENTRY();
    if (_cmd_command_active) {
        _cmd_async_pending = true;
    }
    QF_CRIT_EXIT();
}

void cmd_async_complete(void)
{
    QF_CRIT_ENTRY();
    _cmd_async_pending = false;
    QF_CRIT_EXIT();
    cmd_try_complete();
}

static void cmd_dma_start(cmd_tx_slot_t const* slot)
{
    DMA_InitType DMA_InitStructure;

    DMA_DeInit(USART_CMD_DMA_TX);
    DMA_InitStructure.PeriphAddr     = (uint32_t)&USART_CMD->DAT;
    DMA_InitStructure.MemAddr        = (uint32_t)slot->buf;
    DMA_InitStructure.Direction      = DMA_DIR_PERIPH_DST;
    DMA_InitStructure.BufSize        = slot->len;
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.CircularMode   = DMA_MODE_NORMAL;
    DMA_InitStructure.Priority       = DMA_PRIORITY_HIGH;
    DMA_InitStructure.Mem2Mem        = DMA_M2M_DISABLE;

    USART_ClrIntPendingBit(USART_CMD, USART_INT_TXC);
    DMA_Init(USART_CMD_DMA_TX, &DMA_InitStructure);
    DMA_RequestRemap(USART_CMD_DMA_TX_MAP, DMA, USART_CMD_DMA_TX, ENABLE);
    USART_EnableDMA(USART_CMD, USART_DMAREQ_TX, ENABLE);
    DMA_EnableChannel(USART_CMD_DMA_TX, ENABLE);
    USART_ConfigInt(USART_CMD, USART_INT_TXC, ENABLE);
}

static void cmd_dma_rx_restart(void)
{
    /* Stopping a circular transfer does not rewind the memory address. */
    USART_CMD_DMA_RX->MADDR = (uint32_t)_cmd_buf_rx;
    DMA_SetCurrDataCounter(USART_CMD_DMA_RX, CMD_BUF_LEN);
    DMA_EnableChannel(USART_CMD_DMA_RX, ENABLE);
}

static void cmd_dma_on_tx_complete(void)
{
    uint8_t const tail = _cmd_tx_tail;
    uint8_t const next = cmd_tx_next_index(tail);

    USART_ClrIntPendingBit(USART_CMD, USART_INT_TXC);
    if (next == _cmd_tx_head) {
        _cmd_tx_tail = next;
        _cmd_tx_active = false;
        USART_ConfigInt(USART_CMD, USART_INT_TXC, DISABLE);
        DMA_EnableChannel(USART_CMD_DMA_TX, DISABLE);
        cmd_try_complete();
        return;
    }

    _cmd_tx_tail = next;
    cmd_dma_start(&_cmd_tx_queue[next]);
}

void USART_CMD_IRQHandler(void)
{
    uint16_t cmd_len = 0U;

    if (USART_GetIntStatus(USART_CMD, USART_INT_IDLEF) != RESET) {
        (void)USART_CMD->STS;
        (void)USART_CMD->DAT;
        DMA_EnableChannel(USART_CMD_DMA_RX, DISABLE); // Disable DMA to get current count
        cmd_len = CMD_BUF_LEN - DMA_GetCurrDataCounter(USART_CMD_DMA_RX);
        if (!_cmd_start_string_received) {
            // Check for "Start" string in the received data
            for (uint16_t i = 0; i < cmd_len; i++) {
                if (_cmd_buf_rx[i] == CMD_START_STRING[_cmd_start_match_pos]) {
                    _cmd_start_match_pos++;
                    if (_cmd_start_match_pos == strlen(CMD_START_STRING)) {
                        APP_LOG_DEBUG("Start CMD");
                        _cmd_start_string_received = true;
                        _cmd_start_match_pos       = 0;
                        QEvt_ctor(&_cmd_evt_prepare, VALVE_CMD_PREPARE_SIG);
                        (void)QACTIVE_POST_X(AO_ValveConf, &_cmd_evt_prepare, 1U, 0U);
                        break;
                    }
                } else {
                    _cmd_start_match_pos = 0; // Reset if mismatch
                }
            }
        } else if (!_cmd_set_name) {
            for (uint16_t i = 0; i < cmd_len; i++) {
                if (_cmd_buf_rx[i] == CMD_DEVICE_NAME[_cmd_start_match_pos]) {
                    _cmd_start_match_pos++;
                    if (_cmd_start_match_pos == strlen(CMD_DEVICE_NAME)) {
                        APP_LOG_DEBUG("Set Device Name OK");
                        _cmd_set_name        = true;
                        _cmd_start_match_pos = 0;
                        break;
                    }
                } else {
                    _cmd_start_match_pos = 0; // Reset if mismatch
                }
            }
        } else {
            // Process the received command
            // Assuming command ends with '\n'
            if (cmd_len > 0U && cmd_len < CMD_BUF_LEN &&
                                            _cmd_buf_rx[cmd_len - 1U] == '\n')
            {
                (void)cmd_post_received(_cmd_buf_rx, cmd_len);
            }
        }

        cmd_dma_rx_restart();
    }
    if ((USART_GetFlagStatus(USART_CMD, USART_FLAG_OREF) != RESET) ||
        (USART_GetFlagStatus(USART_CMD, USART_FLAG_NEF) != RESET) ||
        (USART_GetFlagStatus(USART_CMD, USART_FLAG_PEF) != RESET) ||
        (USART_GetFlagStatus(USART_CMD, USART_FLAG_FEF) != RESET)) {
        /*Read the sts register first,and the read the DAT register to clear the all error flag*/
        (void)USART_CMD->STS;
        (void)USART_CMD->DAT;
        /* Under normal circumstances, all error flags will be cleared when the upper data is read and will not be executed here;
           users can add their own processing according to the actual scenario. */
    }

    if (USART_GetIntStatus(USART_CMD, USART_INT_TXC) != RESET) {
        if (_cmd_tx_active) {
            cmd_dma_on_tx_complete();
        } else {
            USART_ClrIntPendingBit(USART_CMD, USART_INT_TXC);
            USART_ConfigInt(USART_CMD, USART_INT_TXC, DISABLE);
        }
    }
}

void cmd_init(void)
{
    BLE_PWR_HIGH;

    uart_init(BLE);
    USART_Enable(USART_CMD, DISABLE);

    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA, ENABLE);
    memset(_cmd_buf_rx, 0, sizeof(_cmd_buf_rx));
    _cmd_start_match_pos       = 0U;
    _cmd_start_string_received = false;
    _cmd_set_name              = false;
    _cmd_tx_head               = 0U;
    _cmd_tx_tail               = 0U;
    _cmd_tx_active             = false;
    _cmd_command_active        = false;
    _cmd_async_pending         = false;
    _cmd_execution_done        = false;
    cmd_valve_info_reset();
    USART_EnableDMA(USART_CMD, USART_DMAREQ_TX, DISABLE);
    DMA_DeInit(USART_CMD_DMA_TX);

    DMA_InitType DMA_InitStructure;
    DMA_DeInit(USART_CMD_DMA_RX);
    DMA_InitStructure.PeriphAddr     = (uint32_t)&USART_CMD->DAT;
    DMA_InitStructure.MemAddr        = (uint32_t)_cmd_buf_rx;
    DMA_InitStructure.Direction      = DMA_DIR_PERIPH_SRC;
    DMA_InitStructure.BufSize        = CMD_BUF_LEN;
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.CircularMode   = DMA_MODE_CIRCULAR;
    DMA_InitStructure.Priority       = DMA_PRIORITY_VERY_HIGH;
    DMA_InitStructure.Mem2Mem        = DMA_M2M_DISABLE;
    DMA_Init(USART_CMD_DMA_RX, &DMA_InitStructure);
    DMA_RequestRemap(USART_CMD_DMA_RX_MAP, DMA, USART_CMD_DMA_RX, ENABLE);
    USART_EnableDMA(USART_CMD, USART_DMAREQ_RX, ENABLE);
    DMA_EnableChannel(USART_CMD_DMA_RX, ENABLE);
    USART_Enable(USART_CMD, ENABLE);
    USART_ConfigInt(USART_CMD, USART_INT_IDLEF, ENABLE); // Enable USART IDLE interrupt
    USART_ConfigInt(USART_CMD, USART_INT_TXC, DISABLE);

    NVIC_EnableIRQ(USART_CMD_IRQn); // Enable USART2 interrupt

    if (RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_SFTRSTF) == SET && cmd_module_already_on) {
        APP_LOG_DEBUG("System is reboot from software ...");
        _cmd_start_string_received = true;
        _cmd_set_name              = true;
    }
}

void cmd_deinit(void)
{
    BLE_PWR_LOW;
    _cmd_start_string_received = false;
    _cmd_set_name              = false;
    cmd_module_already_on      = false;
    _cmd_start_match_pos       = 0;
    USART_ConfigInt(USART_CMD, USART_INT_TXC, DISABLE);
    USART_EnableDMA(USART_CMD, USART_DMAREQ_TX, DISABLE);
    _cmd_tx_active = false;
    _cmd_tx_head   = 0U;
    _cmd_tx_tail   = 0U;
    _cmd_command_active = false;
    _cmd_async_pending  = false;
    _cmd_execution_done = false;
    cmd_valve_info_reset();
    DMA_EnableChannel(USART_CMD_DMA_RX, DISABLE);
    DMA_DeInit(USART_CMD_DMA_RX);
    DMA_DeInit(USART_CMD_DMA_TX);
    NVIC_DisableIRQ(USART_CMD_IRQn);
    uart_deinit(BLE);
}

void cmd_dma_transmit(const uint8_t* data, uint16_t len)
{
    if (data == NULL || len == 0U) {
        APP_LOG_ERROR("Invalid DMA transmit buffer.");
        return;
    }

    if (len > CMD_TX_BUF_LEN) {
        APP_LOG_ERROR("DMA transmit data too long: %u.", len);
        return;
    }

    QF_CRIT_ENTRY();
    uint8_t const head = _cmd_tx_head;
    uint8_t const next = cmd_tx_next_index(head);
    if (next == _cmd_tx_tail) {
        QF_CRIT_EXIT();
        return;
    }

    cmd_tx_slot_t* const slot = &_cmd_tx_queue[head];
    memcpy(slot->buf, data, len);
    slot->len = len;
    _cmd_tx_head = next;
    if (!_cmd_tx_active) {
        _cmd_tx_active = true;
        cmd_dma_start(slot);
    }
    QF_CRIT_EXIT();
}

void cmd_response(uint16_t result)
{
    uint8_t response[2] = {
        (uint8_t)(result & 0xffU),
        (uint8_t)(result >> 8),
    };
    cmd_dma_transmit(response, sizeof(response));
}

// 解析并执行命令
void cmd_execute(char* input)
{
    char* const original_input = input;

    // 去除换行符(如果有)
    input[strcspn(input, "\r\n")] = 0;

    // 跳过前导空格
    while (*input == ' ') input++;

    // 空命令处理
    if (*input == '\0') {
        goto _clear;
    }

    // 分割参数
    char* args[64]; // 最多支持64个参数
    int   argc = 0;

    char* token = strtok(input, " ");
    while (token != NULL && argc < 32) {
        args[argc++] = token;
        token        = strtok(NULL, " ");
    }

    if (argc == 0) {
        goto _clear;
    }

    // 查找命令
    for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (strcmp(args[0], commands[i].name) == 0) {
            // 检查参数数量
            if (argc - 1 > commands[i].max_args) {
                APP_LOG_ERROR("Error: Too many arguments for command '%s'. Max is %d.",
                              commands[i].name, commands[i].max_args);
                goto _clear;
            }

            // 调用处理函数(跳过命令名)
            int result = commands[i].handler(argc - 1, args + 1);
            if (result < 0) {
                cmd_response(CMD_ERR);
            } else {
                cmd_response(CMD_OK);
            }

            goto _clear;
        }
    }

    APP_LOG_ERROR("Error: Unknown command '%s'", args[0]);

_clear:
    cmd_rx_release(original_input);
    QF_CRIT_ENTRY();
    _cmd_execution_done = true;
    QF_CRIT_EXIT();
    cmd_try_complete();
}

int cmd_ping(int argc, char** argv)
{
    (void)argc; // 未使用参数
    (void)argv; // 未使用参数

    APP_LOG_DEBUG("Pong!");
    return 0;
}

void cmd_set_name(void)
{
    static char name[32];
    snprintf(name, sizeof(name), "AT+NAME%s%c\r\n", CMD_DEVICE_NAME, '\0');
    APP_LOG_DEBUG("Device name set to: %s", CMD_DEVICE_NAME);

    // 发送设置名称命令
    cmd_dma_transmit((const uint8_t*)name, strlen(name));
}
