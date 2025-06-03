#include "qpc.h"
#include "bsp.h"
#include "cmd.h"

#define USART_BLE            USART2
#define USART_BLE_IRQHandler USART2_IRQHandler
#define MAX_CMD_LEN         256
#define MAX_CMDS            16

typedef struct {
    const char *name;
    void (*callback)(const char *args);
} CmdEntry;

static char cmd_buffer[MAX_CMD_LEN] = {0};
static uint16_t cmd_pos = 0;
static CmdEntry cmd_table[MAX_CMDS] = {0};
static uint8_t cmd_count = 0;

void cmd_register(const char *name, void (*callback)(const char *args)) {
    if (cmd_count < MAX_CMDS) {
        cmd_table[cmd_count].name = name;
        cmd_table[cmd_count].callback = callback;
        cmd_count++;
    }
}

static void cmd_process(const char *cmd) {
    char cmd_name[32] = {0};
    const char *args = NULL;
    
    // 提取命令名和参数
    sscanf(cmd, "%31s", cmd_name);
    args = cmd + strlen(cmd_name);
    while (*args == ' ') args++;
    
    // 查找并执行回调
    for (uint8_t i = 0; i < cmd_count; i++) {
        if (strcmp(cmd_name, cmd_table[i].name) == 0) {
            cmd_table[i].callback(args);
            return;
        }
    }
}

void USART_BLE_IRQHandler() {
    if (USART_GetIntStatus(USART_BLE, USART_INT_RXDNE) != RESET) {
        char ch = (char)USART_ReceiveData(USART_BLE);
        
        if (ch == '\n' || ch == '\r') {
            if (cmd_pos > 0) {
                cmd_buffer[cmd_pos] = '\0';                
                cmd_pos = 0;
                
            }
        } else if (cmd_pos < MAX_CMD_LEN - 1) {
            cmd_buffer[cmd_pos++] = ch;
        }
        
        USART_ClrIntPendingBit(USART_BLE, USART_INT_RXDNE);
        USART_ClrFlag(USART_BLE, USART_FLAG_RXDNE);
    }
}