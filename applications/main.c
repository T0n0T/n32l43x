#include "board.h"
#include "stdio.h"
#include "flash.h"
#include "led.h"
#include "uart.h"
#include "bootloader.h"
#include "bootble.h"

void run_led(void)
{
    led_toggle(LED_3);
}

void assert_failed(const uint8_t* expr, const uint8_t* file, uint32_t line)
{
    printf("Assert failed: %s, file %s, line %d\r\n", expr, file, line);
#ifdef DEBUG /* debug build? */
    cm_backtrace_assert(cmb_get_sp());
    while (1); /* tie the CPU in this endless loop */
#endif
    NVIC_SystemReset(); /* reset the CPU */
}

void main(void)
{
#ifdef DEBUG
    cm_backtrace_init("bootloader", "N32L43x", "1.0.0");
#endif
    board_init();
    led_init();
    uart_init(CONSOLE);
    printf(" \r\n");
    printf("┌──────────────────────────────────────────────┐\r\n");
    printf("│ N32L43x bootloader %s-%s      │\r\n", __DATE__, __TIME__);
    printf("└──────────────────────────────────────────────┘\r\n");
    bootloader_systimer_init();
    bootloader_systimer_add_task(run_led, 1000, true);
    bootloader_wdt_init();
    bootloader_ble_init(bootloader_systimer_millis());
    while (!bootloader_ble_is_finished()) {
        bootloader_wdt_feed();
        bootloader_systimer_run_tasks();
        bootloader_ble_process(bootloader_systimer_millis());
        __WFE();
    }
    if (bootloader_ble_is_ready()) {
        BOOT_LOG_INFO("BLE ready at 115200 baud");
    }
    if (flash_option_get() != UPDATE_FLAG_MASK && app_is_valid(APP_START_ADDR)) {
        bootloader_ble_deinit();
        uint32_t power_off_ms = bootloader_systimer_millis();
        while (bootloader_systimer_millis() - power_off_ms < 100U) {
            bootloader_wdt_feed();
            __WFE();
        }
        SysTick->CTRL = 0;
        SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
        app_run((uint32_t)APP_START_ADDR);
    }
    BOOT_LOG_INFO("A firmware need to flash\r\n");
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
