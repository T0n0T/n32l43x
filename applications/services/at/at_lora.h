#ifndef AT_LORA_H_
#define AT_LORA_H_

#include "at.h"
#include "stdbool.h"
#include "stdint.h"

typedef enum at_lora_cmd_enum {
    AT_LORA_CMD_WAKE,
    AT_LORA_CMD_SET_PID,
    AT_LORA_CMD_SET_NIP,
    AT_LORA_CMD_SET_TYP,
    AT_LORA_CMD_SET_LFR,
    AT_LORA_CMD_SET_LRS,
    AT_LORA_CMD_SET_TPR,
    AT_LORA_CMD_JOIN,
    AT_LORA_CMD_SEND_ASCII,
    AT_LORA_CMD_ENUM_MAX,
} at_lora_cmd_enum_t;

void at_lorawan_init(void);
void at_lorawan_deinit(void);
void at_lorawan_config_prepare(void);
void at_lorawan_send_prepare(char* payload);
bool at_lorawan_is_ready(void);

void at_lorawan_event_post(void);

at_process_result_t at_lorawan_poll(uint32_t tick);

#endif // AT_LORA_H_