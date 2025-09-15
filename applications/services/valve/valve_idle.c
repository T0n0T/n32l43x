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

static void valve_report(void)
{
    static uint32_t current_time = 0;
    static uint8_t  blink_count  = 0;
    current_time++;
    if (current_time >= (blink_count + 1) * 30) {
        led_toggle(LED_1);
        if (pvd_is_power_low) {
            led_toggle(LED_2);
        }
        if (transfer_is_error) {
            led_toggle(LED_3);
        }
        blink_count++;
    }
    if (blink_count == 100) {
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
}