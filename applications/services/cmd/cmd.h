#ifndef __CMD_H__
#define __CMD_H__

#include "cmd_impl.h" // 包含cmd_impl.h

#define CMD_DEVICE_NAME      "AirPressure-CYG\0"

#define USART_CMD            USART2
#define USART_CMD_IRQn       USART2_IRQn
#define USART_CMD_IRQHandler USART2_IRQHandler
#define USART_CMD_DMA_TX     DMA_CH5
#define USART_CMD_DMA_TX_MAP DMA_REMAP_USART2_TX
#define USART_CMD_DMA_RX     DMA_CH6
#define USART_CMD_DMA_RX_MAP DMA_REMAP_USART2_RX
#define CMD_BUF_LEN          64U

#define CMD_OK               0xcafe
#define CMD_ERR              0xdead
#define CMD_DEFINE(func)     int cmd_##func(int argc, char** argv);

#define CMD_DEFINE_LIST      {                     \
    {"config_refactory", cmd_config_refactory, 0}, \
    {"config_write", cmd_config_write, 1},         \
    {"config_read", cmd_config_read, 0},           \
    {"ping", cmd_ping, 0},                         \
    {"reboot", cmd_reboot, 0},                     \
    {"update", cmd_update, 0},                     \
    {"valve_info", cmd_valve_info, 1},             \
}

extern bool cmd_module_already_on;

// 命令处理函数类型定义
typedef int (*cmd_handler_t)(int argc, char** argv);

// 命令结构体
typedef struct {
    const char*   name;     // 命令名称
    cmd_handler_t handler;  // 处理函数
    int           max_args; // 最大参数个数
} command_t;

void cmd_init(void);
void cmd_deinit(void);
void cmd_set_name(void);
void cmd_execute(char* input);
void cmd_dma_transmit(const uint8_t* data, uint16_t len);

CMD_DEFINE(config_refactory)
CMD_DEFINE(config_write)
CMD_DEFINE(config_read)
CMD_DEFINE(ping)
CMD_DEFINE(reboot)
CMD_DEFINE(update)
CMD_DEFINE(valve_info)
CMD_DEFINE(valve_tuning)

#endif // __CMD_H__