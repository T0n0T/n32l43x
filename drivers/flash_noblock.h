#ifndef __FLASH_NOBLOCK_H__
#define __FLASH_NOBLOCK_H__

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Flash操作类型定义 */
typedef enum {
    FLASH_OP_ERASE_PAGE,      /* 页擦除 */
    FLASH_OP_PROGRAM_WORD,    /* 字编程 */
    FLASH_OP_ERASE_OPTION,    /* 选项字节擦除 */
    FLASH_OP_PROGRAM_OPTION,  /* 选项字节编程 */
} flash_op_type_t;

/* Flash操作状态定义 */
typedef enum {
    FLASH_STATE_IDLE,         /* 空闲状态 */
    FLASH_STATE_INIT,         /* 初始化状态 */
    FLASH_STATE_UNLOCK,       /* 解锁Flash */
    FLASH_STATE_CLEAR_FLAG,   /* 清除标志位 */
    FLASH_STATE_WAIT_READY,   /* 等待就绪 */
    FLASH_STATE_SET_OP_TYPE,  /* 设置操作类型 */
    FLASH_STATE_SET_ADDRESS,  /* 设置地址 */
    FLASH_STATE_SET_DATA,     /* 设置数据 */
    FLASH_STATE_START_OP,     /* 启动操作 */
    FLASH_STATE_WAIT_COMPLETE,/* 等待操作完成 */
    FLASH_STATE_LOCK,         /* 锁定Flash */
    FLASH_STATE_DONE,         /* 操作完成 */
    FLASH_STATE_ERROR,        /* 操作错误 */
} flash_state_t;

/* Flash操作结果定义 */
typedef enum {
    FLASH_RESULT_PENDING,     /* 操作进行中 */
    FLASH_RESULT_SUCCESS,     /* 操作成功 */
    FLASH_RESULT_ERROR,       /* 操作失败 */
    FLASH_RESULT_TIMEOUT,     /* 操作超时 */
} flash_result_t;

/* Flash操作请求结构体 */
typedef struct {
    flash_op_type_t type;     /* 操作类型 */
    uint32_t address;         /* 操作地址 */
    uint32_t data;            /* 操作数据 */
    uint16_t option_data;     /* 选项字节数据 */
} flash_request_t;

/* Flash状态机结构体 */
typedef struct {
    flash_state_t state;      /* 当前状态 */
    flash_request_t request;  /* 当前请求 */
    flash_result_t result;    /* 操作结果 */
    uint32_t timeout;         /* 超时计数器 */
    void (*callback)(flash_result_t result, void *user_data); /* 完成回调 */
    void *user_data;          /* 用户数据 */
} flash_fsm_t;

/* 初始化Flash状态机 */
void flash_fsm_init(flash_fsm_t *fsm);

/* 启动Flash操作 */
bool flash_fsm_start(flash_fsm_t *fsm, const flash_request_t *request,
                    void (*callback)(flash_result_t result, void *user_data),
                    void *user_data);

/* 处理Flash状态机 */
void flash_fsm_process(flash_fsm_t *fsm);

/* 取消当前操作 */
void flash_fsm_cancel(flash_fsm_t *fsm);

/* 获取当前操作结果 */
static inline flash_result_t flash_fsm_get_result(const flash_fsm_t* fsm)
{
    assert_param(fsm != NULL);
    return fsm->result;
}

/* 检查操作是否完成 */
static inline bool flash_fsm_is_complete(const flash_fsm_t* fsm)
{
    assert_param(fsm != NULL);
    return (fsm->state == FLASH_STATE_DONE || fsm->state == FLASH_STATE_ERROR);
}

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_NOBLOCK_H__ */