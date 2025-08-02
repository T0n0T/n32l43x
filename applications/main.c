#include <rtthread.h>
#include <rtdevice.h>
#include <stdio.h>
#include "board.h"

int main(void)
{
    rt_pin_mode(GET_PIN(A, 8), PIN_MODE_OUTPUT);
    while (1)
    {
        rt_thread_mdelay(500);
        rt_pin_write(GET_PIN(A, 8), 1);
        rt_thread_mdelay(500);
        rt_pin_write(GET_PIN(A, 8), 0);
        // rt_kprintf("Hello RT-Thread!\n"); // Print message to console
    }
    return 0;
}

