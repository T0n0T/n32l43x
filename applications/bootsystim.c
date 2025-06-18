#include "bootloader.h"
#include "n32l43x.h" // For SysTick_Config and SystemCoreClock

#define SYSTICK_PERIOD_MS 10 // SysTick中断周期，单位毫秒

// 定义定时器任务结构体
typedef struct {
    void (*task_func)(void); // 任务函数指针
    uint32_t interval_ms;    // 定时周期，单位毫秒
    uint32_t current_count;  // 当前计数
    uint8_t  is_active;      // 任务是否激活
    uint8_t  is_running;     // 任务是否正在执行
    bool     is_periodic;    // 是否周期性任务 (1: 周期性, 0: 单次)
} systimer_task_t;

// 最多维护三个定时器任务
#define MAX_SYSTIMER_TASKS 3
static systimer_task_t systimer_tasks[MAX_SYSTIMER_TASKS];

void SysTick_Handler(void)
{
    for (int i = 0; i < MAX_SYSTIMER_TASKS; i++) {
        if (systimer_tasks[i].is_active) {
            systimer_tasks[i].current_count += SYSTICK_PERIOD_MS;
        }
    }
}

/**
 * @brief 运行软件定时器任务
 *        此函数应在主循环中周期性调用
 */
void bootloader_systimer_run_tasks(void)
{
    for (int i = 0; i < MAX_SYSTIMER_TASKS; i++) {
        if (systimer_tasks[i].is_active && !systimer_tasks[i].is_running) {
            if (systimer_tasks[i].current_count >= systimer_tasks[i].interval_ms) {
                systimer_tasks[i].current_count = 0; // 重置计数
                if (systimer_tasks[i].task_func) {
                    systimer_tasks[i].is_running = 1; // 标记任务正在执行
                    systimer_tasks[i].task_func();
                    systimer_tasks[i].is_running = 0; // 标记任务执行完毕
                }
                if (!systimer_tasks[i].is_periodic) {
                    systimer_tasks[i].is_active = 0; // 单次任务执行完毕后停用
                }
            }
        }
    }
}

/**
 * @brief 添加一个软件定时器任务
 *
 * @param task_func 任务函数
 * @param interval_ms 定时周期，单位毫秒
 * @return int 0成功，-1失败（无可用槽位）
 */
/**
 * @brief 添加一个软件定时器任务
 *
 * @param task_func 任务函数
 * @param interval_ms 定时周期，单位毫秒
 * @param is_periodic 是否周期性任务 (1: 周期性, 0: 单次)
 * @return int 0成功，-1失败（无可用槽位）
 */
int bootloader_systimer_add_task(void (*task_func)(void), uint32_t interval_ms, bool is_periodic)
{
    for (int i = 0; i < MAX_SYSTIMER_TASKS; i++) {
        if (!systimer_tasks[i].is_active) {
            systimer_tasks[i].task_func     = task_func;
            systimer_tasks[i].interval_ms   = interval_ms;
            systimer_tasks[i].current_count = 0;
            systimer_tasks[i].is_active     = 1;
            systimer_tasks[i].is_running    = 0; // 初始化为未运行
            systimer_tasks[i].is_periodic   = is_periodic;
            return i; // 返回任务索引
        }
    }
    return -1; // 没有可用槽位
}

/**
 * @brief 重置指定索引的定时器任务
 *
 * @param task_index 任务索引
 * @return int 0成功，-1失败（索引无效或任务未激活）
 */
int bootloader_systimer_reset_task(int task_index)
{
    if (task_index >= 0 && task_index < MAX_SYSTIMER_TASKS && systimer_tasks[task_index].is_active) {
        systimer_tasks[task_index].current_count = 0;
        return 0;
    }
    return -1;
}

void bootloader_systimer_init(void)
{
    // 初始化所有任务为非激活状态
    for (int i = 0; i < MAX_SYSTIMER_TASKS; i++) {
        systimer_tasks[i].is_active  = 0;
        systimer_tasks[i].is_running = 0;
    }
    // 配置SysTick中断，每SYSTICK_PERIOD_MS毫秒触发一次
    SysTick_Config(SystemCoreClock / (1000 / SYSTICK_PERIOD_MS));
}