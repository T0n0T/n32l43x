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
    rt_pm_default_set(PM_SLEEP_MODE_DEEP); // Set default sleep mode to none
        // rt_pm_request(PM_SLEEP_MODE_DEEP); // Request no sleep mode
    while (1) {
        rt_pm_request(PM_SLEEP_MODE_NONE);
        rt_pin_write(RUN_LED, PIN_HIGH);
        rt_pm_release(PM_SLEEP_MODE_NONE);
        rt_thread_mdelay(1000);
        rt_pm_request(PM_SLEEP_MODE_NONE);
        rt_pin_write(RUN_LED, PIN_LOW);
        rt_pm_release(PM_SLEEP_MODE_NONE);
        rt_thread_mdelay(1000); 
    }
    return 0;
}

