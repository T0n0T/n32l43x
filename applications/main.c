#include <rtthread.h>
#include <rtdevice.h>
#include "drv_gpio.h"
#include <stdio.h>
#include <elog.h>

#define RUN_LED GET_PIN(A, 8) // Define the pin for the LED

int main(void)
{
    rt_pin_mode(RUN_LED, PIN_MODE_OUTPUT); // Set pin A8 as output
    printf("Hello RT-Thread!\n");      // Print message to console
    while (1) {
        rt_pin_write(RUN_LED, PIN_HIGH);
        rt_thread_mdelay(1000);
        rt_pin_write(RUN_LED, PIN_LOW);
        rt_thread_mdelay(1000); 
    }
    return 0;
}

