#include "qpc.h" // QP/C real-time embedded framework
#include "bsp.h" // Board Support Package interface
#include "stdio.h"
#include "string.h"
#include "cmd.h"
#include "valve.h"
#include "log.h"

static QEvt              _cmd_evt_prepare;
static ValveEvt          _cmd_evt;
static uint8_t           _cmd_buf_rx[CMD_BUF_LEN];
static uint8_t           _cmd_buf_tx[CMD_BUF_LEN * 2];
static uint8_t           _cmd_start_match_pos;
static bool              _cmd_start_string_received;
static bool              _cmd_set_name;
static volatile uint16_t _cmd_len; // Change to uint16_t for length
static const command_t   commands[]       = CMD_DEFINE_LIST;
static const char*       CMD_START_STRING = "Start";
bool                     cmd_module_already_on;

void USART_CMD_IRQHandler(void)
{
    if (USART_GetIntStatus(USART_CMD, USART_INT_IDLEF) != RESET) {
        (void)USART_CMD->STS;
        (void)USART_CMD->DAT;
        DMA_EnableChannel(USART_CMD_DMA_RX, DISABLE); // Disable DMA to get current count
        _cmd_len = CMD_BUF_LEN - DMA_GetCurrDataCounter(USART_CMD_DMA_RX);

        if (!_cmd_start_string_received) {
            // Check for "Start" string in the received data
            for (uint16_t i = 0; i < _cmd_len; i++) {
                if (_cmd_buf_rx[i] == CMD_START_STRING[_cmd_start_match_pos]) {
                    _cmd_start_match_pos++;
                    if (_cmd_start_match_pos == strlen(CMD_START_STRING)) {
                        APP_LOG_DEBUG("Start CMD");
                        _cmd_start_string_received = true;
                        _cmd_start_match_pos       = 0;
                        QEvt_ctor(&_cmd_evt_prepare, VALVE_CMD_PREPARE_SIG);
                        QACTIVE_POST(AO_ValveConf, &_cmd_evt_prepare, 0U);
                        break;
                    }
                } else {
                    _cmd_start_match_pos = 0; // Reset if mismatch
                }
            }
        } else if (!_cmd_set_name) {
            for (uint16_t i = 0; i < _cmd_len; i++) {
                if (_cmd_buf_rx[i] == CMD_DEVICE_NAME[_cmd_start_match_pos]) {
                    _cmd_start_match_pos++;
                    if (_cmd_start_match_pos == strlen(CMD_DEVICE_NAME)) {
                        APP_LOG_DEBUG("Set Device Name OK");
                        _cmd_set_name              = true;
                        _cmd_start_match_pos       = 0;
                        break;
                    }
                } else {
                    _cmd_start_match_pos = 0; // Reset if mismatch
                }
            }
        }
        else {
            // Process the received command
            // Assuming command ends with '\n'
            if (_cmd_len > 0 && _cmd_buf_rx[_cmd_len - 1] == '\n') {
                _cmd_buf_rx[_cmd_len] = '\0'; // Null-terminate the string
                QACTIVE_POST(AO_ValveConf, &_cmd_evt.super, 0U);
            }
        }

        DMA_SetCurrDataCounter(USART_CMD_DMA_RX, CMD_BUF_LEN); // Reset DMA buffer size
        DMA_EnableChannel(USART_CMD_DMA_RX, ENABLE);           // Re-enable DMA
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
}

void cmd_init(void)
{
    BLE_PWR_HIGH;

    uart_init(BLE);
    USART_Enable(USART_CMD, DISABLE);

    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA, ENABLE);
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

    NVIC_EnableIRQ(USART_CMD_IRQn); // Enable USART2 interrupt

    memset(_cmd_buf_rx, 0, sizeof(_cmd_buf_rx));
    _cmd_len                   = 0;     // Initialize command buffer length
    _cmd_start_match_pos       = 0;     // Initialize Start string match position
    _cmd_start_string_received = false; // Initialize Start string received flag
    _cmd_set_name              = false; // Initialize Device Name set flag

    QEvt_ctor(&_cmd_evt.super, VALVE_CMD_PARSE_SIG);
    _cmd_evt.msg     = _cmd_buf_rx; // 设置消息指针指向命令缓冲区
    _cmd_evt.evtType = VALVE_CMD;   // 设置事件类型为命令解析

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
    DMA_EnableChannel(USART_CMD_DMA_RX, DISABLE);
    DMA_DeInit(USART_CMD_DMA_RX);
    DMA_DeInit(USART_CMD_DMA_TX);
    NVIC_DisableIRQ(USART_CMD_IRQn);
    uart_deinit(BLE);
}

void cmd_dma_transmit(const uint8_t* data, uint16_t len)
{
    memcpy(_cmd_buf_tx, data, len); // Copy data to transmit buffer
    DMA_InitType DMA_InitStructure;
    DMA_DeInit(USART_CMD_DMA_TX);
    DMA_InitStructure.PeriphAddr     = (uint32_t)&USART_CMD->DAT;
    DMA_InitStructure.MemAddr        = (uint32_t)_cmd_buf_tx;
    DMA_InitStructure.Direction      = DMA_DIR_PERIPH_DST;
    DMA_InitStructure.BufSize        = len;
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.CircularMode   = DMA_MODE_NORMAL;
    DMA_InitStructure.Priority       = DMA_PRIORITY_HIGH;
    DMA_InitStructure.Mem2Mem        = DMA_M2M_DISABLE;
    DMA_Init(USART_CMD_DMA_TX, &DMA_InitStructure);
    DMA_RequestRemap(USART_CMD_DMA_TX_MAP, DMA, USART_CMD_DMA_TX, ENABLE);
    USART_EnableDMA(USART_CMD, USART_DMAREQ_TX, ENABLE);
    DMA_EnableChannel(USART_CMD_DMA_TX, ENABLE);
}

void cmd_response(uint16_t result)
{
    uart_putc(BLE, (uint8_t)(result & 0xff));
    uart_putc(BLE, (uint8_t)(result >> 8));
}

// 解析并执行命令
void cmd_execute(char* input)
{
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
    _cmd_len = 0;
    memset(_cmd_buf_rx, 0, sizeof(_cmd_buf_rx)); // Clear command buffer
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
    snprintf(name, sizeof(name), "AT+NAME%s\r\n", CMD_DEVICE_NAME);
    APP_LOG_DEBUG("Device name set to: %s", CMD_DEVICE_NAME);

    // 发送设置名称命令
    cmd_dma_transmit((const uint8_t*)name, strlen(name));
}
