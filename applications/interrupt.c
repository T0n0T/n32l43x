#include "board.h"
#include "led.h"

volatile uint32_t tick_count;

void SysTick_Handler(void)
{
    tick_count++;
    if (tick_count % 1000 == 0) {
        led_toggle(LED_1); // Toggle LED1 every second
    }   
}
