#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include <stdint.h>
#include "n32l43x.h"

#define APP_START_ADDR   0x08005000
#define UPDATE_FLAG_ADDR 0x0801F800
#define UPDATE_FLAG_MASK 0x12345678

// application function
void app_run(uint32_t app_addr);
bool app_is_valid(uint32_t app_addr);

// systimer function
void bootloader_systimer_init(void);
void bootloader_systimer_run_tasks(void);
int  bootloader_systimer_add_task(void (*task_func)(void), uint32_t interval_ms, bool is_periodic);
int  bootloader_systimer_reset_task(int task_index);

// watch dog function
void bootloader_wdt_init(void);
void bootloader_wdt_feed(void);

// dfu function
void bootloader_dfu_init(void);

#endif // __BOOTLOADER_H