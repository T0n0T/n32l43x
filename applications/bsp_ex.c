#include "bsp.h"
#include "log.h"
#include "guard.h"
#include "qpc.h"
#include "valve.h"

typedef enum task_desc_enum {
    TASK_CMD = 1,
    TASK_COUNTER,
    TASK_DAILY,
    TASK_HANDLE,
    TASK_CONFIG,
    TASK_PERSIST,
    TASK_PVD,
} task_desc_enum_t;

typedef struct task_desc {
    char* desc_string;
};

int g_cmd_id;
int g_counter_id;
int g_handle_id;
int g_persist_id;
int g_transfer_id;

static void cmd_guard_handle(int task_id, void* data)
{
    assert_param(false);
}

static void counter_guard_handle(int task_id, void* data)
{
    assert_param(false);
}

static void handle_guard_handle(int task_id, void* data)
{
    assert_param(false);
}

static void persist_guard_handle(int task_id, void* data)
{
    assert_param(false);
}

static void transfer_guard_handle(int task_id, void* data)
{
    assert_param(false);
}

static void low_power_gpio_optimize(void)
{
    GPIO_InitType GPIO_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);

    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA |
                                RCC_APB2_PERIPH_GPIOB |
                                RCC_APB2_PERIPH_GPIOC |
                                RCC_APB2_PERIPH_GPIOD,
                            ENABLE);
    GPIO_InitStructure.Pin       = BLE_PWR_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitPeripheral(BLE_PWR_PORT, &GPIO_InitStructure);
    // BLE_PWR_LOW;

    GPIO_InitStructure.Pin       = AT_PWR_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitPeripheral(AT_PWR_PORT, &GPIO_InitStructure);
    // AT_PWR_LOW;
}

void BSP_init_ext(void)
{
    low_power_gpio_optimize();
    guard_init();
    g_cmd_id = guard_register(GUARD_TYPE_DISCRETE, cmd_guard_handle, NULL);
    assert_param(g_cmd_id >= 0);
    g_counter_id = guard_register(GUARD_TYPE_CONTINUOUS, counter_guard_handle, NULL);
    assert_param(g_counter_id >= 0);
    g_handle_id = guard_register(GUARD_TYPE_DISCRETE, handle_guard_handle, NULL);
    assert_param(g_handle_id >= 0);
    g_persist_id = guard_register(GUARD_TYPE_DISCRETE, persist_guard_handle, NULL);
    assert_param(g_persist_id >= 0);
    g_transfer_id = guard_register(GUARD_TYPE_DISCRETE, transfer_guard_handle, NULL);
    assert_param(g_transfer_id >= 0);

    // 每个周期按 LPTIM_INTERVAL_MS 计算
    guard_discreate_set_tolerance(g_cmd_id, 1);
    guard_discreate_set_tolerance(g_handle_id, 1);
    guard_discreate_set_tolerance(g_persist_id, 2);
    guard_discreate_set_tolerance(g_transfer_id, 2);
#ifdef DEBUG
    SEGGER_SYSVIEW_Conf();
    extern SEGGER_SYSVIEW_TASKINFO _Q_taskInfo[5];
    _Q_taskInfo[0].TaskID = (uint32_t)AO_ValveCounter;
    _Q_taskInfo[0].sName  = "AO_ValveCounter";
    _Q_taskInfo[0].Prio   = 5U;
    _Q_taskInfo[1].TaskID = (uint32_t)AO_ValveHandler;
    _Q_taskInfo[1].sName  = "AO_ValveHandler";
    _Q_taskInfo[1].Prio   = 4U;
    _Q_taskInfo[2].TaskID = (uint32_t)AO_ValveConf;
    _Q_taskInfo[2].sName  = "AO_ValveConf";
    _Q_taskInfo[2].Prio   = 3U;
    _Q_taskInfo[3].TaskID = (uint32_t)AO_ValvePersist;
    _Q_taskInfo[3].sName  = "AO_ValvePersist";
    _Q_taskInfo[3].Prio   = 2U;
    _Q_taskInfo[4].TaskID = (uint32_t)AO_ValveTransfer;
    _Q_taskInfo[4].sName  = "AO_ValveTransfer";
    _Q_taskInfo[4].Prio   = 1U;
#endif
}

#ifdef QF_ON_CONTEXT_SW
void QF_onContextSw(QActive* prev, QActive* next)
{
    if (prev) {
#ifdef DEBUG
        SEGGER_SYSVIEW_OnTaskStopExec();
#endif
        if (prev == AO_ValveCounter) {
            guard_continuous_report(g_counter_id);
        }
        if (prev == AO_ValveConf) {
            guard_discrete_unmark(g_cmd_id);
        }
        if (prev == AO_ValveHandler) {
            guard_discrete_unmark(g_handle_id);
        }
        if (prev == AO_ValvePersist) {
            guard_discrete_unmark(g_persist_id);
        }
        if (prev == AO_ValveTransfer) {
            guard_discrete_unmark(g_transfer_id);
        }
    }
    if (next) {
#ifdef DEBUG
        SEGGER_SYSVIEW_OnTaskStartExec((uint32_t)next);
#endif
        if (next == AO_ValveConf) {
            guard_discrete_mark(g_cmd_id);
        }
        if (next == AO_ValveHandler) {
            guard_discrete_mark(g_handle_id);
        }
        if (next == AO_ValvePersist) {
            guard_discrete_mark(g_persist_id);
        }
        if (next == AO_ValveTransfer) {
            guard_discrete_mark(g_transfer_id);
        }
    }
}
#endif