#include "guard.h"
#include "lptimer.h"
#include <string.h>

#define GUARD_TASK_NUM      5
#define GUARD_TASK_INTERVAL 1000
#define GUARD_TASK_MASKED   0x80000000  // 任务屏蔽标志位

typedef struct guard_task {
    uint32_t          mask;
    uint32_t          last_time;
    uint32_t          interval;
    guard_type_t      type;
    guard_handle_func handle;
    void*             data;
} guard_task_t;

static guard_task_t guard_task[GUARD_TASK_NUM];

int task_guard_init(void)
{

    return 0;
}

int task_guard_register(guard_type_t type, guard_handle_func handle, void* data)
{
    int task_id = -1;
    
    // 查找一个空闲的任务槽位
    for (int i = 0; i < GUARD_TASK_NUM; i++) {
        if (guard_task[i].handle == NULL) {
            task_id = i;
            break;
        }
    }
    
    // 如果找到了空闲槽位，则初始化任务
    if (task_id > 0) {
        guard_task[task_id].mask = 0;
        guard_task[task_id].last_time = lptimer_tick();
        guard_task[task_id].interval = GUARD_TASK_INTERVAL;
        guard_task[task_id].type = type;
        guard_task[task_id].handle = handle;
        guard_task[task_id].data = data;
    }
    
    return task_id;
}

void task_guard_mask(int task_id)
{
    if (task_id < GUARD_TASK_NUM) {
        guard_task[task_id].mask |= GUARD_TASK_MASKED;
    }
}

void task_guard_unmask(int task_id)
{
    if (task_id < GUARD_TASK_NUM) {
        guard_task[task_id].mask &= ~GUARD_TASK_MASKED;
    }
}

void task_guard_continuous_report(int task_id)
{
    if (task_id < GUARD_TASK_NUM &&
        guard_task[task_id].type == GUARD_TYPE_CONTINUOUS &&
        !(guard_task[task_id].mask & GUARD_TASK_MASKED)) {
        // 更新任务的最后报告时间
        guard_task[task_id].last_time = lptimer_tick();
    }
}

void task_guard_discrete_mark(int task_id)
{
    if (task_id < GUARD_TASK_NUM &&
        guard_task[task_id].type == GUARD_TYPE_DISCRETE &&
        !(guard_task[task_id].mask & GUARD_TASK_MASKED)) {
        // 标记离散任务开始
    }
}

void task_guard_discrete_unmark(int task_id)
{
    if (task_id < GUARD_TASK_NUM &&
        guard_task[task_id].type == GUARD_TYPE_DISCRETE &&
        !(guard_task[task_id].mask & GUARD_TASK_MASKED)) {
        // 标记离散任务结束，更新最后时间
    }
}

void task_guard_custom_report(int task_id, uint32_t error)
{
    if (task_id < GUARD_TASK_NUM &&
        guard_task[task_id].type == GUARD_TYPE_CUSTOM &&
        !(guard_task[task_id].mask & GUARD_TASK_MASKED) &&
        guard_task[task_id].handle != NULL) {
        // 调用自定义处理函数报告错误
    }
}