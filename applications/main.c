#include "board.h"
#include "stdio.h"
#include "flash.h"
#include "led.h"
#include "uart.h"
#include "../hello/hello.h"
#ifdef DEBUG
#include "cm_backtrace.h"
#endif

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
    SysTick_Config(SystemCoreClock / 1000); // 1ms tick
    led_init();
    uart_init(CONSOLE);
    func_instance* func_table = (func_instance*)0x08010000;

    printf("hello %s\r\n", func_table[0].func.const_char_ptr());

    while (1) {

        // Wait for an event.
        __WFE();
        // Clear the internal event register.
        __SEV();
        __WFE();
    }
}
