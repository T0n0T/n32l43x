#include "board.h"
#include "stdio.h"
#include "flash.h"
#include "lptimer.h"
#include "uart.h"
#include "cm_backtrace.h"
#include "bootloader.h"

void main(void)
{
    cm_backtrace_init("bootloader", "N32L43x", "1.0.0");
    board_init();
    lptimer_init();
    uart_init(CONSOLE);
    app_start_final(0x8010000);
}
