#include "string.h"
#include "at.h"
#include "log.h"

void at_fsm_init(at_t* at)
{
    // 初始化状态机
    at->state               = AT_STATE_IDLE;
    at->send_time           = 0;
    at->retry_count         = 0;
    if (at->transfer_init) {
        at->transfer_init();
    }
}

void at_fsm_deinit(at_t* at)
{
    if (at->transfer_deinit) {
        at->transfer_deinit();
    }
}

void at_fsm_copy_buffer(at_t* at, uint8_t* buffer, uint32_t buffer_size)
{
    if (at->current_process_buf && at->state == AT_STATE_WAITING_RESP) {
        memcpy(at->current_process_buf, buffer, buffer_size);
        at->current_process_len = buffer_size;
        at->state               = AT_STATE_PROCESSING_RESP;
    } else {
        APP_LOG_DEBUG("Received AT unknown: %s", buffer);
    }
}

void at_fsm_request_list_set(at_t* at, const at_cmd_t* at_cmd_list, uint32_t at_cmd_list_len)
{
    at->at_cmd_list         = at_cmd_list; // 存储传递的at_cmd参数
    at->at_cmd_list_len     = at_cmd_list_len;
    at->current_process_at  = 0;
    at->current_process_buf = NULL;
    at->current_process_len = 0;
}

/**
 * @brief 处理AT命令的状态机
 *
 * @param at AT模块实例指针
 * @param tick 当前系统tick值
 * @return at_process_result_t
 */
at_process_result_t at_fsm_request_process(at_t* at, uint32_t tick)
{
    // 检查AT命令列表是否有效
    if (at->at_cmd_list == NULL) {
        return AT_PROCESS_COMPLETE; // 没有命令需要处理
    }

    switch (at->state) {
        case AT_STATE_IDLE:
            // 检查是否还有命令需要处理
            if (at->current_process_at >= at->at_cmd_list_len) {
                return AT_PROCESS_COMPLETE; // 所有命令处理完成
            }

            // 重置重试计数
            at->retry_count = 0;
            // 进入发送状态
            at->state = AT_STATE_SENDING;
            break;

        case AT_STATE_SENDING:
            // 发送当前命令
            if (at->at_cmd_list[at->current_process_at].cmd_expr != NULL) {
                if (at->transfer_transmit) {
                    at->transfer_transmit((const uint8_t*)at->at_cmd_list[at->current_process_at].cmd_expr,
                                          strlen(at->at_cmd_list[at->current_process_at].cmd_expr));
                }
                // 记录发送时间
                at->send_time = tick;
                // 进入等待响应状态
                at->state = AT_STATE_WAITING_RESP;
            } else {
                // 命令无效，移动到下一个命令
                at->current_process_at++;
                at->state = AT_STATE_IDLE;
            }
            break;

        case AT_STATE_WAITING_RESP:
            // 检查是否超时
            if ((tick - at->send_time) > at->at_cmd_list[at->current_process_at].timeout) {
                // 超时处理
                at->state = AT_STATE_TIMEOUT_RETRY;
                break;
            }

            break;

        case AT_STATE_PROCESSING_RESP:
            // 处理接收到的响应
            // 跳过前导换行符
            at->current_process_ptr = at->current_process_buf;
            while (*at->current_process_ptr == '\r' || *at->current_process_ptr == '\n')
                at->current_process_ptr++;

            // 跳过前导空格
            while (*at->current_process_ptr == ' ') at->current_process_ptr++;

            // 空回复处理
            if ((*at->current_process_ptr == '\r' && *(at->current_process_ptr + 1) == '\n') ||
                *at->current_process_ptr == '\0') {
                // 空回复，继续等待或移动到下一个命令
                at->current_process_len = 0; // 清空缓冲区
                at->state               = AT_STATE_WAITING_RESP;
                break;
            }

            // 查找匹配的AT回复
            if (strstr(at->at_cmd_list[at->current_process_at].resp_keyword, at->current_process_ptr) != NULL) {
                // 找到匹配的回复
                APP_LOG_DEBUG("Received matching AT reply: %s", at->current_process_ptr);
                if (at->at_cmd_list[at->current_process_at].resp_callback) {
                    int result = at->at_cmd_list[at->current_process_at].resp_callback(at->current_process_ptr);
                    if (result < 0) {
                        // 处理失败
                        at->state = AT_STATE_ERROR;
                    }
                }
                // 清空缓冲区
                at->current_process_len = 0;
                // 移动到下一个命令
                at->current_process_at++;
                // 回到空闲状态，准备处理下一个命令
                at->state = AT_STATE_IDLE;
            } else {
                // 如果没有找到匹配的回复，记录日志
                APP_LOG_DEBUG("Received AT reply: %s", at->current_process_ptr);
                // 清空缓冲区
                at->current_process_len = 0;
                // 继续等待响应
                at->state = AT_STATE_WAITING_RESP;
            }
            break;

        case AT_STATE_TIMEOUT_RETRY:
            // 增加重试计数
            at->retry_count++;

            // 检查是否达到重试上限
            if (at->retry_count >= at->at_cmd_list[at->current_process_at].retry) {
                // 重试次数达到上限，记录错误并进入错误状态
                APP_LOG_ERROR("AT command timed out after %d retries: %s",
                              at->retry_count, at->at_cmd_list[at->current_process_at].cmd_expr);
                // 进入错误状态
                at->state = AT_STATE_ERROR;
            } else {
                // 重新发送命令
                APP_LOG_DEBUG("AT command timeout, retrying (%d/%d): %s",
                              at->retry_count, at->at_cmd_list[at->current_process_at].retry,
                              at->at_cmd_list[at->current_process_at].cmd_expr);
                // 回到发送状态
                at->state = AT_STATE_SENDING;
            }
            break;

        case AT_STATE_ERROR:
            // 错误状态，可以添加错误处理逻辑
            return AT_PROCESS_ERROR; // 返回错误状态

        default:
            // 未知状态，回到空闲状态
            at->state = AT_STATE_IDLE;
            break;
    }

    // 默认返回继续处理
    return AT_PROCESS_CONTINUE;
}
