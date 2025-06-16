#include "qpc.h" // QP/C real-time embedded framework
#include "bsp.h" // Board Support Package interface
#include "stdio.h"
#include "string.h"
#include "cmd.h"
#include "valve.h"

#define BLE_PWR_PORT         GPIOB
#define BLE_PWR_CLK          RCC_APB2_PERIPH_GPIOB
#define BLE_PWR_PIN          GPIO_PIN_6
#define BLE_PWR_HIGH         BLE_PWR_PORT->PBSC = BLE_PWR_PIN;
#define BLE_PWR_LOW          BLE_PWR_PORT->PBC = BLE_PWR_PIN;

#define USART_CMD            USART2
#define USART_CMD_IRQn       USART2_IRQn
#define USART_CMD_IRQHandler USART2_IRQHandler
#define CMD_BUF_LEN          64U

extern int              ble_flag;

static ValveEvt         _cmd_evt;
static uint8_t          _cmd_ch;
static uint8_t          _cmd_buf[CMD_BUF_LEN];
static volatile uint8_t _cmd_pos;
static const command_t  commands[] = CMD_DEFINE_LIST;

// void USART_CMD_IRQHandler(void)
// {
//     // process the UART2 interrupt
//     if (USART_GetIntStatus(USART_CMD, USART_INT_RXDNE) != RESET) {
//         /* Read one byte from the receive data register */
//         if (_cmd_pos < CMD_BUF_LEN - 1) {
//             _cmd_buf[_cmd_pos] = USART_ReceiveData(USART_CMD);
//             _cmd_pos++;
//             if (_cmd_buf[_cmd_pos - 1] == '\n') {
//                 // process the command
//                 // _cmd_buf[_cmd_pos] = '\0'; // null-terminate the string
//                 // Reset the command buffer position
//                 QACTIVE_POST(AO_ValveConf, &_cmd_evt.super, 0U);
//             }
//         }else{
//             USART_ReceiveData(USART_CMD);
//         }
//         USART_ClrIntPendingBit(USART_CMD, USART_INT_RXDNE);
//         USART_ClrFlag(USART_CMD, USART_FLAG_RXDNE);
//     }
//     if ((USART_GetFlagStatus(USART_CMD, USART_FLAG_OREF) != RESET) ||
//         (USART_GetFlagStatus(USART_CMD, USART_FLAG_NEF) != RESET) ||
//         (USART_GetFlagStatus(USART_CMD, USART_FLAG_PEF) != RESET) ||
//         (USART_GetFlagStatus(USART_CMD, USART_FLAG_FEF) != RESET)) {
//         /*Read the sts register first,and the read the DAT register to clear the all error flag*/
//         (void)USART_CMD->STS;
//         (void)USART_CMD->DAT;
//         printf("error happened\r\n");
//         /* Under normal circumstances, all error flags will be cleared when the upper data is read and will not be executed here;
//            users can add their own processing according to the actual scenario. */
//     }
// }

void DMA_Channel6_IRQHandler(void)
{
    if (DMA_GetFlagStatus(DMA_FLAG_TC6, DMA) != RESET) {
        if (_cmd_pos < CMD_BUF_LEN - 1) {
            _cmd_buf[_cmd_pos] = _cmd_ch;
            _cmd_pos++;
            if (_cmd_buf[_cmd_pos - 1] == '\n') {
                QACTIVE_POST(AO_ValveConf, &_cmd_evt.super, 0U);
            }
            DMA_ClrIntPendingBit(DMA_INT_TXC6, DMA);
            DMA_ClearFlag(DMA_FLAG_TC6, DMA);
        }
    }
}

void cmd_init(void)
{
    GPIO_InitType GPIO_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);
    RCC_EnableAPB2PeriphClk(BLE_PWR_CLK, ENABLE);
    GPIO_InitStructure.Pin       = BLE_PWR_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitPeripheral(BLE_PWR_PORT, &GPIO_InitStructure);

    BLE_PWR_HIGH;

    ble_flag = true;
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

    QEvt_ctor(&_cmd_evt.super, VALVE_CMD_PARSE_SIG);
    _cmd_evt.msg     = _cmd_buf;  // 设置消息指针指向命令缓冲区
    _cmd_evt.evtType = VALVE_CMD; // 设置事件类型为命令解析
}

void cmd_deinit(void)
{
    BLE_PWR_LOW;
    ble_flag = false;
    NVIC_DisableIRQ(USART_CMD_IRQn);
    uart_deinit(BLE_SERIAL);
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
                printf("Error: Too many arguments for command '%s'. Max is %d.\r\n",
                       commands[i].name, commands[i].max_args);
                goto _clear;
            }

            // 调用处理函数(跳过命令名)
            commands[i].handler(argc - 1, args + 1);
            goto _clear;
        }
    }

    printf("Error: Unknown command '%s'\r\n", args[0]);

_clear:
    _cmd_pos = 0;
    memset(_cmd_buf, 0, sizeof(_cmd_buf)); // 清空命令缓冲区
}

int cmd_reboot(int argc, char** argv)
{
    (void)argc; // 未使用参数
    (void)argv; // 未使用参数
    printf("System is rebooting...\r\n");
    NVIC_SystemReset(); // 调用系统重启函数
    return 0;
}