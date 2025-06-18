#include "board.h"
#include "stdio.h"
#include "flash.h"
#include "led.h"
#include "uart.h"
#include "cm_backtrace.h"
#include "bootloader.h"

void run_led(void)
{
    led_toggle(LED_3);
}

void main(void)
{
#ifdef DEBUG
    cm_backtrace_init("bootloader", "N32L43x", "1.0.0");
#endif
    board_init();
    led_init();
    uart_init(CONSOLE);
    bootloader_systimer_init();
    bootloader_systimer_add_task(run_led, 1000, true);
    bootloader_wdt_init();

    if (*(uint32_t*)UPDATE_FLAG_ADDR != UPDATE_FLAG_MASK && app_is_valid(APP_START_ADDR)) {
        app_run((uint32_t)APP_START_ADDR);
    }
    bootloader_dfu_init();
    while (1) {
        bootloader_wdt_feed();
        bootloader_systimer_run_tasks();
        // Wait for an event.
        __WFE();
        // Clear the internal event register.
        __SEV();
        __WFE();
    }
}
