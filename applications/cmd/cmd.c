#include "qpc.h" // QP/C real-time embedded framework
#include "bsp.h" // Board Support Package interface
#include "stdio.h"
#include "string.h"
#include "log.h"
#include "cmd.h"
#include "hello.h"

#define USART_CMD            USART2
#define USART_CMD_IRQn       USART2_IRQn
#define USART_CMD_IRQHandler USART2_IRQHandler
#define CMD_BUF_LEN          64U

static CmdEvt           _cmd_evt;
static uint8_t          _cmd_ch;
static uint8_t          _cmd_buf[CMD_BUF_LEN];
static volatile uint8_t _cmd_pos;
static const command_t  commands[] = CMD_DEFINE_LIST;

void DMA_Channel6_IRQHandler(void)
{
    if (DMA_GetFlagStatus(DMA_FLAG_TC6, DMA) != RESET) {
        if (_cmd_pos < CMD_BUF_LEN - 1) {
            _cmd_buf[_cmd_pos] = _cmd_ch;
            _cmd_pos++;
            if (_cmd_buf[_cmd_pos - 1] == '\n') {
                QACTIVE_POST(AO_Hello, &_cmd_evt.super, 0U);
            }
        }
        DMA_ClrIntPendingBit(DMA_INT_TXC6, DMA);
        DMA_ClearFlag(DMA_FLAG_TC6, DMA);
    }
}

void cmd_init(void)
{
    // Enable USART/DMA interrupt
    uart_init(BLE_SERIAL);
    USART_Enable(USART_CMD, DISABLE);

    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA, ENABLE);
    DMA_InitType DMA_InitStructure;
    DMA_DeInit(DMA_CH6);
    DMA_InitStructure.PeriphAddr     = (USART2_BASE + 0x04);
    DMA_InitStructure.MemAddr        = (uint32_t)&_cmd_ch;
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
    USART_EnableDMA(USART_CMD, USART_DMAREQ_RX, ENABLE);
    DMA_EnableChannel(DMA_CH6, ENABLE);
    USART_Enable(USART_CMD, ENABLE);

    NVIC_SetPriority(DMA_Channel6_IRQn, 0U);
    NVIC_EnableIRQ(DMA_Channel6_IRQn);

    memset(_cmd_buf, 0, sizeof(_cmd_buf));
    _cmd_pos = 0; // 初始化命令缓冲区位置

    // Initialize the command event
    QEvt_ctor(&_cmd_evt.super, USER_COMMAND_SIG);
    _cmd_evt.msg = _cmd_buf;
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
            commands[i].handler(argc - 1, args + 1);
            goto _clear;
        }
    }

    APP_LOG_ERROR("Error: Unknown command '%s'", args[0]);

_clear:
    _cmd_pos = 0;
    memset(_cmd_buf, 0, sizeof(_cmd_buf)); // 清空命令缓冲区
}

int cmd_reboot(int argc, char** argv)
{
    (void)argc; // 未使用参数
    (void)argv; // 未使用参数
    APP_LOG_INFO("System is rebooting...");
    NVIC_SystemReset(); // 调用系统重启函数
    return 0;
}

int cmd_update(int argc, char** argv)
{
    (void)argc; // 未使用参数
    (void)argv; // 未使用参数
    flash_erase_option();
    flash_program_option(0x1234);
    APP_LOG_INFO("System  updating...");
    NVIC_SystemReset();
    // 这里可以添加实际的更新逻辑
    return 0;
}
