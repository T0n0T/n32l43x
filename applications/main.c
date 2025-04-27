#include "board.h"
#include "led.h"

int main(void)
{
    board_init();
    led_init();
    while (1) {
        led_toggle(LED_RUN);
        for (volatile int i = 0; i < SystemCoreClock / 50; i++)
            __NOP();
    }

    return 0;
}
