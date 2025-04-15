#include <rtthread.h>
#include <stdio.h>

int main(void)
{
    while (1)
    {
        rt_thread_mdelay(1000); // Delay for 1 second
        rt_kprintf("Hello RT-Thread!\n"); // Print message to console
    }
    return 0;
}

