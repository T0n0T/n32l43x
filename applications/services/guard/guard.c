#include "guard.h"
#include "lptimer.h"
#include <string.h>

#define GUARD_INTERVAL_MS 500
#define GUARD_TASK_NUM    8
#define GUARD_TASK_MASK   0x80000000 // 任务屏蔽标志位

typedef struct guard_continuous_priv {
    volatile uint32_t last_report;
    uint32_t          total_report;
} guard_continuous_priv_t;

typedef struct guard_decrete_priv {
    volatile uint32_t mark_period;
    volatile uint32_t tolerance_period;
    bool              finished;
} guard_decrete_priv_t;

typedef struct guard_task {
    uint32_t     mask;
    guard_type_t type;
    union {
        guard_continuous_priv_t continus_priv;
        guard_decrete_priv_t    decrete_priv;
        void*                   custom_priv;
    } record;
    guard_handle_func handle;
    void*             data;
} guard_task_t;

typedef struct guard_object {
    bool              is_sleep;
    volatile uint32_t current_period;
    guard_task_t      task[GUARD_TASK_NUM];
} guard_object_t;

static guard_object_t guard_ins;

static uint32_t guard_atomic_load(volatile uint32_t* key)
{
    uint32_t val;
    do {
        val = __LDREXW(key);
    } while ((__STREXW(val, key)));
    return val;
}

static void guard_atomic_set(volatile uint32_t* key, uint32_t value)
{
    do {
        __LDREXW(key);
    } while ((__STREXW(value, key)));
}

static void guard_atomic_set_mask(volatile uint32_t* key, uint32_t mask)
{
    uint32_t temp;
    do {
        temp = __LDREXW(key);
    } while ((__STREXW(temp | mask, key)));
}

static void guard_atomic_clear_mask(volatile uint32_t* key, uint32_t mask)
{
    uint32_t temp;
    do {
        temp = __LDREXW(key);
    } while ((__STREXW(temp & ~mask, key)));
}

static void guard_atomic_set_bool(volatile bool* key, bool value)
{
    uint32_t temp;
    do {
        temp = __LDREXW((volatile uint32_t*)key);
    } while ((__STREXW(value, (volatile uint32_t*)key)));
}

static void guard_atomic_set_ptr(void* volatile* key, void* value)
{
    uint32_t temp;
    do {
        temp = __LDREXW((volatile uint32_t*)key);
    } while ((__STREXW((uint32_t)value, (volatile uint32_t*)key)));
}

static void guard_atomic_pluse(volatile uint32_t* key)
{
    uint32_t temp;
    do {
        temp = __LDREXW(key);
    } while ((__STREXW(temp + 1, key)));
}

static void guard_process(void)
{
    for (int i = 0; i < GUARD_TASK_NUM; i++) {
        if (!(guard_ins.task[i].mask & GUARD_TASK_MASK) && !guard_ins.is_sleep &&
            guard_ins.task[i].handle) {
            switch (guard_ins.task[i].type) {
                case GUARD_TYPE_CONTINUOUS:
                    if ((guard_ins.current_period != guard_ins.task[i].record.continus_priv.last_report) &&
                        (guard_ins.task[i].record.continus_priv.total_report == 0)) {
                        guard_ins.task[i].handle(i, guard_ins.task[i].data);
                    }
                    guard_atomic_set(&guard_ins.task[i].record.continus_priv.total_report, 0);
                    break;

                case GUARD_TYPE_DISCRETE:
                    // Check for overflow condition: if the difference is too large, it means current_period has overflowed
                    // or the task has not been marked for a very long time
                    if ((guard_ins.current_period - guard_ins.task[i].record.decrete_priv.mark_period >=
                         guard_ins.task[i].record.decrete_priv.tolerance_period) &&
                        (guard_ins.current_period - guard_ins.task[i].record.decrete_priv.mark_period < 0x80000000) && // Check for reasonable time difference
                        !guard_ins.task[i].record.decrete_priv.finished) {
                        guard_ins.task[i].handle(i, guard_ins.task[i].data);
                    }
                    break;
                case GUARD_TYPE_CUSTOM:
                    if (guard_ins.task[i].record.custom_priv != NULL) {
                        guard_ins.task[i].handle(i, guard_ins.task[i].data);
                    }
                    break;
                default:
                    break;
            }
        }
    }
    // 喂狗
    guard_atomic_pluse(&guard_ins.current_period);
}

int guard_init(void)
{
    lptimer_init();
    lptimer_start(LPTIMER_MS_TO_TICKS(GUARD_INTERVAL_MS), guard_process);
    // 初始化看门狗
    return 0;
}

int guard_register(guard_type_t type, guard_handle_func handle, void* data)
{
    int task_id = -1;

    // 查找一个空闲的任务槽位
    for (int i = 0; i < GUARD_TASK_NUM; i++) {
        if (guard_ins.task[i].type == 0) {
            task_id = i;
            break;
        }
    }

    // 如果找到了空闲槽位,则初始化任务,并屏蔽
    if (task_id > 0) {
        guard_ins.task[task_id].mask   = 0;
        guard_ins.task[task_id].type   = type;
        guard_ins.task[task_id].handle = handle;
        guard_ins.task[task_id].data   = data;
    }

    return task_id;
}

void guard_mask(int task_id)
{
    if (task_id < GUARD_TASK_NUM) {
        guard_atomic_set_mask(&guard_ins.task[task_id].mask, GUARD_TASK_MASK);
    }
}

void guard_unmask(int task_id)
{
    if (task_id < GUARD_TASK_NUM) {
        guard_atomic_clear_mask(&guard_ins.task[task_id].mask, GUARD_TASK_MASK);
    }
}

void guard_sleep(void)
{
    guard_atomic_set_bool(&guard_ins.is_sleep, true);
}

void guard_wakeup(void)
{
    guard_atomic_set_bool(&guard_ins.is_sleep, false);
}

void guard_continuous_report(int task_id)
{
    if (task_id < GUARD_TASK_NUM &&
        guard_ins.task[task_id].type == GUARD_TYPE_CONTINUOUS) {
        // 更新任务的最后报告时间
        guard_atomic_set(&guard_ins.task[task_id].record.continus_priv.last_report,
                         guard_ins.current_period);
        guard_ins.task[task_id].record.continus_priv.total_report++;
    }
}

void guard_discreate_set_tolerance(int task_id, uint32_t tolerance_period)
{
    if (task_id < GUARD_TASK_NUM &&
        guard_ins.task[task_id].type == GUARD_TYPE_DISCRETE) {
        // 设置离散任务的容忍时间
        guard_ins.task[task_id].record.decrete_priv.tolerance_period = tolerance_period;
    }
}

void guard_discrete_mark(int task_id)
{
    if (task_id < GUARD_TASK_NUM &&
        guard_ins.task[task_id].type == GUARD_TYPE_DISCRETE) {
        // 标记离散任务开始
        guard_atomic_set(&guard_ins.task[task_id].record.decrete_priv.mark_period,
                         guard_ins.current_period);
        guard_atomic_set_bool(&guard_ins.task[task_id].record.decrete_priv.finished, false);
    }
}

void guard_discrete_unmark(int task_id)
{
    if (task_id < GUARD_TASK_NUM &&
        guard_ins.task[task_id].type == GUARD_TYPE_DISCRETE) {
        // 标记离散任务结束，更新最后时间
        guard_atomic_set_bool(&guard_ins.task[task_id].record.decrete_priv.finished, true);
    }
}

void guard_custom_report(int task_id, int error)
{
    if (task_id < GUARD_TASK_NUM &&
        guard_ins.task[task_id].type == GUARD_TYPE_CUSTOM) {
        // 调用自定义处理函数报告错误
        guard_atomic_set_ptr(&guard_ins.task[task_id].record.custom_priv, (void*)error);
    }
}

void guard_custom_reset(int task_id)
{
    if (task_id < GUARD_TASK_NUM &&
        guard_ins.task[task_id].type == GUARD_TYPE_CUSTOM) {
        // 重置自定义任务的错误状态
        guard_atomic_set_ptr(&guard_ins.task[task_id].record.custom_priv, NULL);
    }
}
