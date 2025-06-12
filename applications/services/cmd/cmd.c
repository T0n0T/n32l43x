#include "qpc.h" // QP/C real-time embedded framework
#include "bsp.h" // Board Support Package interface
#include "stdio.h"
#include "string.h"
#include "cmd.h"
#include "valve.h"

#define BLE_PWR_PORT GPIOB
#define BLE_PWR_CLK  RCC_APB2_PERIPH_GPIOB
#define BLE_PWR_PIN  GPIO_PIN_6
#define BLE_PWR_HIGH BLE_PWR_PORT->PBSC = BLE_PWR_PIN;
#define BLE_PWR_LOW  BLE_PWR_PORT->PBC = BLE_PWR_PIN;

#define USART_CMD            USART2
#define USART_CMD_IRQn       USART2_IRQn
#define USART_CMD_IRQHandler USART2_IRQHandler
#define CMD_BUF_LEN          64U

static ValveEvt           _cmd_evt;
static uint8_t            _cmd_buf[CMD_BUF_LEN];
static volatile uint8_t   _cmd_pos;
static const command_t  commands[] = CMD_DEFINE_LIST;

void USART_CMD_IRQHandler(void)
{
    // process the UART2 interrupt
    if (USART_GetIntStatus(USART_CMD, USART_INT_RXDNE) != RESET) {
        /* Read one byte from the receive data register */
        printf("Received: %c\r\n", USART_ReceiveData(USART_CMD));
        // if (_cmd_pos < CMD_BUF_LEN - 1) {
        //     _cmd_buf[_cmd_pos] = USART_ReceiveData(USART_CMD);
        //     _cmd_pos++;
        //     if (_cmd_buf[_cmd_pos - 1] == '\n' || _cmd_buf[_cmd_pos - 1] == '\r') {
        //         // process the command
        //         _cmd_buf[_cmd_pos] = '\0'; // null-terminate the string
        //         // Reset the command buffer position
        //         QACTIVE_POST(AO_ValveConf, &_cmd_evt.super, 0U);
        //         _cmd_pos = 0;
        //     }
        // }

        USART_ClrIntPendingBit(USART_CMD, USART_INT_RXDNE);
        USART_ClrFlag(USART_CMD, USART_FLAG_RXDNE);
    }
}

void cmd_init(void)
{
    memset(&_cmd_buf, 0, sizeof(_cmd_buf));
    QEvt_ctor(&_cmd_evt.super, VALVE_CMD_PARSE_SIG);
    _cmd_evt.msg = _cmd_buf; // 设置消息指针指向命令缓冲区
    _cmd_evt.evtType = VALVE_CMD; // 设置事件类型为命令解析
    uart_init(BLE_SERIAL);
    uart_control(BLE_SERIAL, USART_INT_RXDNE, true);
    NVIC_EnableIRQ(USART_CMD_IRQn);

    GPIO_InitType GPIO_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);
    RCC_EnableAPB2PeriphClk(BLE_PWR_CLK, ENABLE);
    GPIO_InitStructure.Pin       = BLE_PWR_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitPeripheral(BLE_PWR_PORT, &GPIO_InitStructure);

    BLE_PWR_HIGH;
}

void cmd_deinit(void)
{
    BLE_PWR_LOW;
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
        return;
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
        return;
    }

    // 查找命令
    for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (strcmp(args[0], commands[i].name) == 0) {
            // 检查参数数量
            if (argc - 1 > commands[i].max_args) {
                printf("Error: Too many arguments for command '%s'. Max is %d.\n",
                       commands[i].name, commands[i].max_args);
                return;
            }

            // 调用处理函数(跳过命令名)
            commands[i].handler(argc - 1, args + 1);
            return;
        }
    }

    printf("Error: Unknown command '%s'\n", args[0]);
}

int cmd_add(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: add <num1> <num2>\n");
        return -1;
    }

    int a = atoi(argv[0]);
    int b = atoi(argv[1]);
    printf("%d + %d = %d\n", a, b, a + b);
    return 0;
}

int cmd_hello(int argc, char** argv)
{
    if (argc == 0) {
        printf("Hello, world!\n");
    } else {
        printf("Hello, %s!\n", argv[0]);
    }
    return 0;
}