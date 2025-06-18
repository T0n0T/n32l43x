#include "board.h"
#include "stdio.h"
#include "flash.h"
#include "lptimer.h"
#include "uart.h"
#include "cm_backtrace.h"

static void jump_to_app(uint32_t app_addr);

void main(void)
{
    cm_backtrace_init("bootloader", "N32L43x", "1.0.0");    
    board_init();
    lptimer_init();
    uart_init(CONSOLE);    
    jump_to_app(0x08004000); // Jump to application at address 0x08004000
    while (1);
}

void jump_to_app(uint32_t app_addr)
{
    typedef void (*app_func_t)(void);
    uint32_t   stk_addr = *((__IO uint32_t*)app_addr);
    app_func_t app_func = (app_func_t)(*((__IO uint32_t*)(app_addr + 4)));

    if ((((uint32_t)app_func & 0xff000000) != 0x08000000) || (((stk_addr & 0x2ff00000) != 0x20000000))) {
        printf("No legitimate application.\r\n");
        return;
    }

    printf("Jump to application running ... \r\n");

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    for (int i = 0; i < 128; i++) {
        NVIC_DisableIRQ((IRQn_Type)i);
        NVIC_ClearPendingIRQ((IRQn_Type)i);
    }
    
    SCB->VTOR = app_addr; // Set vector table to application address
    __set_CONTROL(0);
    __set_MSP(stk_addr);

    app_func(); // Jump to application running

    while (1) {
        // 如果跳转失败，可以在这里添加错误处理或指示
    }
}