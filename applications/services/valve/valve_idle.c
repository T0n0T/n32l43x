#include "qpc.h"
#include "bsp.h"
#include "valve.h"
#include "cmd.h"
#include "log.h"
#include "led.h"
#ifdef DEBUG
#include "SEGGER_SYSVIEW.h"
#endif
#ifdef USE_LORAWAN
#include "at_lora.h"
#endif
static uint32_t last_status;

static void valve_report(void)
{
    static uint32_t current_time = 0;
    static uint8_t  blink_count  = 0;
    current_time++;
    if (current_time >= (blink_count + 1) * 10) {
        led_toggle(LED_1);
        if (pvd_is_power_low) {
            led_toggle(LED_2);
        }
        if (transfer_is_error) {
            led_toggle(LED_3);
        }
        blink_count++;
    }
    if (blink_count == 10) {
        current_time     = 0;
        blink_count      = 0;
        run_is_reporting = false;
    }
}

void valve_idle(void)
{
#ifdef DEBUG
    SEGGER_SYSVIEW_OnIdle();
#endif
    if (run_is_reporting) {
        valve_report();
    }

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