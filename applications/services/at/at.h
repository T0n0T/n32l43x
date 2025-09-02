#ifndef __AT_H__
#define __AT_H__

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

#define AT_CMD(CMD_NUM)      (&at_cmd_list[CMD_NUM])
#define AT_CMD_NAME(CMD_NUM) (#CMD_NUM)
#define AT_CMD_BUFFER_SIZE   128

typedef struct at_cmd_s {
    char*    cmd_desc;
    char*    cmd_expr;
    char*    resp_keyword;
    uint32_t timeout;
    uint32_t retry;
    int (*resp_callback)(char* resp);
} at_cmd_t;

// AT状态机状态枚举
typedef enum {
    AT_STATE_IDLE,            // 空闲状态
    AT_STATE_SENDING,         // 发送命令状态
    AT_STATE_WAITING_RESP,    // 等待响应状态
    AT_STATE_PROCESSING_RESP, // 处理响应状态
    AT_STATE_TIMEOUT_RETRY,   // 超时重试状态
    AT_STATE_ERROR            // 错误状态
} at_state_t;

// AT处理结果枚举
typedef enum {
    AT_PROCESS_COMPLETE, // 处理完成
    AT_PROCESS_CONTINUE, // 继续处理
    AT_PROCESS_ERROR     // 处理出错
} at_process_result_t;

typedef struct at_s {
    const at_cmd_t* at_cmd_list;
    uint32_t        at_cmd_list_len;
    uint32_t        current_process_at;
    char*           current_process_buf;
    char*           current_process_ptr;
    uint32_t        current_process_len;

    // 状态机相关字段
    at_state_t state;       // 当前状态
    uint32_t   send_time;   // 命令发送时间
    uint32_t   retry_count; // 重试次数

    void (*transfer_init)(void);
    void (*transfer_deinit)(void);
    void (*transfer_transmit)(const uint8_t* data, uint16_t len);
} at_t;

void                at_fsm_init(at_t* at);
void                at_fsm_deinit(at_t* at);
void                at_fsm_copy_buffer(at_t* at, uint8_t* buffer, uint32_t buffer_size);
void                at_fsm_request_list_set(at_t* at, const at_cmd_t* at_cmd_list, uint32_t at_cmd_list_len);
at_process_result_t at_fsm_request_process(at_t* at, uint32_t tick);

#endif /* __AT_H__ */
