#ifndef __GUARD_H__
#define __GUARD_H__

#include "stdint.h"

typedef void (*guard_handle_func)(int task_id, void* data);

typedef enum guard_type {
    GUARD_TYPE_CONTINUOUS = 1, // 持续任务
    GUARD_TYPE_DISCRETE,   // 离散任务
    GUARD_TYPE_CUSTOM,     // 自定义类型任务
} guard_type_t;

/**
 * @brief 守护任务处理函数
 */
void guard_process(void);

/**
 * @brief 初始化任务守护模块
 * @return 成功返回0，失败返回非0错误码
 */
int guard_init(void);

/**
 * @brief 注册任务的守护处理函数
 * @param task_id 任务ID
 * @param type 守护类型
 * @param handle 守护事件处理函数指针
 * @param data 自定义数据指针
 * @return 成功返回0，失败返回非0错误码
 */
int guard_register(guard_type_t type, guard_handle_func handle, void* data);

/**
 * @brief 屏蔽指定任务的守护功能
 * @param task_id 任务ID
 */
void guard_mask(int task_id);

/**
 * @brief 解除指定任务的守护功能屏蔽
 * @param task_id 任务ID
 */
void guard_unmask(int task_id);

/**
 * @brief 指示守护模块将要进入睡眠模式
 */
void guard_sleep(void);

/**
 * @brief 指示守护模块已经从睡眠中唤醒
 */
void guard_wakeup(void);

/**
 * @brief 持续任务报告状态
 * @param task_id 任务ID
 */
void guard_continuous_report(int task_id);

/**
 * @brief 离散任务设置容忍时间
 * @param task_id 任务ID
 * @param tolerance_period 容忍时间
 */
void guard_discreate_set_tolerance(int task_id, uint32_t tolerance_period);

/**
 * @brief 离散任务标记起点
 * @param task_id 任务ID
 */
void guard_discrete_mark(int task_id);

/**
 * @brief 离散任务标记终点
 * @param task_id 任务ID
 */
void guard_discrete_unmark(int task_id);

/**
 * @brief 第三方报告自定义任务的运行故障
 * @param task_id 任务ID
 * @param error 故障错误码（自定义含义）
 */
void guard_custom_report(int task_id, int error);

/**
 * @brief 第三方重置自定义任务的故障状态
 * @param task_id 任务ID
 */
void guard_custom_reset(int task_id);
#endif