#include <rtthread.h>
#include <rtdevice.h>
#include "drv_gpio.h"
#include <stdio.h>

#define RUN_LED GET_PIN(A, 8) // Define the pin for the LED

int main(void)
{
#ifdef DEBUG
    #include <n32l40x_dbg.h>
    DBG_ConfigPeriph(DBG_SLEEP | DBG_STOP | DBG_STDBY, ENABLE);
#endif
    rt_pin_mode(RUN_LED, PIN_MODE_OUTPUT); // Set pin A8 as output
    rt_kprintf("Hello RT-Thread!\n");      // Print message to console

    while (1) {
        rt_pin_write(RUN_LED, PIN_HIGH); // Set pin A8 high
        rt_thread_mdelay(1000); // Delay for 1 second
        rt_pin_write(RUN_LED, PIN_LOW); // Set pin A8 low
        rt_thread_mdelay(1000); // Delay for 1 second
        // rt_kprintf("Hello RT-Thread!\n"); // Print message to console
    }
    return 0;
}

