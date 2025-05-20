
/**
 * @file    hall_encoder.c
 * @brief   6霍尔传感器+6磁铁旋转编码器，带容错的方向映射表实现
 */

#include "board.h"
#include "hall.h"

// ============================== 硬件配置 ==============================
#define TICKS_PER_ROTATION 6 // 每圈的跳变次数（6磁铁×2边沿）

// ======================== 方向映射表 ========================
typedef struct {
    uint8_t from; // 起始状态
    uint8_t to;   // 目标状态
    int8_t  dir;  // 方向 (+1正转, -1反转)
} TransitionRule;

// 预定义的合法跳变规则（单/双传感器触发）
static const TransitionRule rules[] = {
    // 正转序列（单→单）
    {0x01, 0x02, +1},
    {0x02, 0x04, +1},
    {0x04, 0x08, +1},
    {0x08, 0x10, +1},
    {0x10, 0x20, +1},
    {0x20, 0x01, +1}, // 跨边界
    // 正转序列（单→双）
    {0x01, 0x03, +0},
    {0x02, 0x06, +0},
    {0x04, 0x0C, +0},
    {0x08, 0x18, +0},
    {0x10, 0x30, +0},
    {0x20, 0x21, +0},
    // 正转序列（双→单）
    {0x03, 0x02, +1},
    {0x06, 0x04, +1},
    {0x0C, 0x08, +1},
    {0x18, 0x10, +1},
    {0x30, 0x20, +1},
    {0x21, 0x01, +1},
    // 正转序列（双→双）
    {0x03, 0x07, +1},
    {0x06, 0x0E, +1},
    {0x0C, 0x18, +1},
    {0x18, 0x30, +1},
    {0x30, 0x21, +1},
    {0x21, 0x03, +1},
    // 反转序列（对称定义）
    {0x02, 0x01, -1},
    {0x04, 0x02, -1},
    {0x08, 0x04, -1},
    {0x10, 0x08, -1},
    {0x20, 0x10, -1},
    {0x01, 0x20, -1}, // 跨边界
    {0x03, 0x01, -0},
    {0x06, 0x02, -0},
    {0x0C, 0x04, -0},
    {0x18, 0x08, -0},
    {0x30, 0x10, -0},
    {0x21, 0x20, -0},
    {0x02, 0x03, -1},
    {0x04, 0x06, -1},
    {0x08, 0x0C, -1},
    {0x10, 0x18, -1},
    {0x20, 0x30, -1},
    {0x01, 0x21, -1},
    {0x07, 0x03, -1},
    {0x0E, 0x06, -1},
    {0x18, 0x08, -1},
    {0x30, 0x18, -1},
    {0x21, 0x30, -1},
    {0x03, 0x21, -1}
};

// ======================== 核心逻辑 ========================
// 读取传感器状态（6-bit编码）
static uint8_t read_sensor_state(void)
{
    uint8_t state = 0;
    for (int i = 0; i < HALL_MAX; i++) {
        if (hall_read(i) == true) {
            state |= (1 << i);
        }
    }
    return state;
}

// 检查是否为合法状态（单或双传感器触发）
static bool is_valid_state(uint8_t* state)
{
    switch (*state) {
        // 单传感器触发
        case 0x01:
        case 0x02:
        case 0x04:
        case 0x08:
        case 0x10:
        case 0x20:
        // 双传感器触发（相邻两个）
        case 0x03:
        case 0x06:
        case 0x0C:
        case 0x18:
        case 0x30:
        case 0x21:
            return true;
        default:
            return false; // 非法状态（如三个传感器同时触发）,无传感器触发
    }
}

// 检查方向（允许1-bit误差）
static int8_t check_direction(uint8_t* old, uint8_t* new)
{
    if (!is_valid_state(new)) return 0;
    // 遍历所有预定义规则
    printf("New State: %02X\r\n", *new);
    for (size_t i = 0; i < sizeof(rules) / sizeof(rules[0]); i++) {
        if (rules[i].from == *old && rules[i].to == *new) {
            *old = *new; // 更新状态
            return rules[i].dir;
        }
    }
    *old = *new; // 更新状态
    return 0; // 非法跳变
}

// 更新旋转计数
static void update_rotation_count(int8_t dir)
{
    static int32_t total_ticks = 0; // 总的跳变计数
    static int32_t rotations   = 0; // 完整圈数
    static int32_t position    = 0; // 当前位置(0-5)

    total_ticks += dir;

    // 更新位置计数
    position = (total_ticks % TICKS_PER_ROTATION + TICKS_PER_ROTATION) % TICKS_PER_ROTATION;
    // 更新圈数
    rotations = total_ticks / TICKS_PER_ROTATION;

    // 打印详细信息
    printf("Position: %d/6 (%.1f°), Rotations: %d, Total Ticks: %d\r\n",
           position,
           (position * 360.0f / TICKS_PER_ROTATION),
           rotations,
           total_ticks);
}

// ======================== 公开接口 ========================
void rotary_encoder_update(void)
{
    static uint8_t last_state = 0;
    uint8_t        new_state  = read_sensor_state();

    if (new_state != last_state) {
        int8_t dir = check_direction(&last_state, &new_state);
        if (dir != 0) {
            update_rotation_count(dir);
        } else {
            // 处理错误（如复位状态）
        }
    }
}