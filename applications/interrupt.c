#include "board.h"

volatile uint32_t tick_count;

void SysTick_Handler(void)
{
    tick_count++;
}
