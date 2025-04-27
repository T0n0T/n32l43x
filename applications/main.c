#include <rtthread.h>
#include <rtdevice.h>
#include "drv_gpio.h"
#include <stdio.h>

#define RUN_LED GET_PIN(B, 4)

int main(void)
{
    rt_pin_mode(RUN_LED, PIN_MODE_OUTPUT);
    rt_pin_write(RUN_LED, PIN_LOW);

    while (1) {
        rt_pin_write(RUN_LED, PIN_HIGH);
        rt_thread_mdelay(1000);
        rt_pin_write(RUN_LED, PIN_LOW);
        rt_thread_mdelay(1000); 
    }
    return 0;
}

