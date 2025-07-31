#include "qpc.h"
#include "bsp.h"
#include "valve.h"
#include "cmd.h"
#include "log.h"
#include "led.h"
#ifdef DEBUG
#include "SEGGER_SYSVIEW.h"
#endif

static uint32_t last_status;

// 状态机状态定义
typedef enum {
    STATUS_IDLE = 0, // 空闲状态，等待10秒超时
    STATUS_BLINKING, // 闪烁状态
} valve_status_state_t;

// 状态机数据结构
typedef struct {
    valve_status_state_t state;
    uint32_t             blink_start_time;
    uint8_t              blink_count;
} valve_status_ctx_t;

static valve_status_ctx_t status_ctx;

static void valve_check_status(void)
{
    static uint32_t current_time = 0;

    // 由于1ms调用一次，直接递增计数器
    current_time++;

    switch (status_ctx.state) {
        case STATUS_IDLE:
            // 检查是否达到10秒（10000ms）
            if (current_time >= 10000) {
                status_ctx.state            = STATUS_BLINKING;
                status_ctx.blink_start_time = current_time;
                status_ctx.blink_count      = 0;
            }
            break;

        case STATUS_BLINKING:
            // 检查是否完成500ms闪烁（500ms / 50ms = 10次）
            // 每50ms执行一次LED反转
            if ((current_time - status_ctx.blink_start_time) >= (status_ctx.blink_count + 1) * 50) {
                led_toggle(LED_1);
                status_ctx.blink_count++;
            }
            if (status_ctx.blink_count == 10) {
                // 闪烁完成，返回空闲状态
                status_ctx.state = STATUS_IDLE;
                current_time     = 0;
            }
            break;

        default:
            break;
    }
}

void valve_idle(void)
{
#ifdef DEBUG
    SEGGER_SYSVIEW_OnIdle();
#endif

    valve_check_status();

    if (last_status != global_valve_value->current_status) {
#ifdef DEBUG
        SEGGER_SYSVIEW_MarkStart(VALVE_IDLE);
#endif
        if (global_valve_value->current_status == VALVE_STATUS_ON) {
            LCD->RAM_COM[LCD_RAM1_COM0] = 0x00000000;
            LCD->RAM_COM[LCD_RAM1_COM1] = 0x00000000;
            LCD->RAM_COM[LCD_RAM1_COM2] = 0x00005000;
            __LCD_CLEAR_FLAG(LCD_FLAG_UDD_CLEAR);
            __LCD_UPDATE_REQUEST();
#ifdef USE_MODBUS
            ucSCoilBuf[0] |= (1 << 0);
#endif
        } else if (global_valve_value->current_status == VALVE_STATUS_OFF) {
            LCD->RAM_COM[LCD_RAM1_COM0] = 0x00005000;
            LCD->RAM_COM[LCD_RAM1_COM1] = 0x00000000;
            LCD->RAM_COM[LCD_RAM1_COM2] = 0x00000000;
            __LCD_CLEAR_FLAG(LCD_FLAG_UDD_CLEAR);
            __LCD_UPDATE_REQUEST();
#ifdef USE_MODBUS
            ucSCoilBuf[0] &= ~(1 << 0);
#endif
        }
#ifdef DEBUG
        SEGGER_SYSVIEW_MarkStop(VALVE_IDLE);
#endif
        last_status = global_valve_value->current_status;
    }
}