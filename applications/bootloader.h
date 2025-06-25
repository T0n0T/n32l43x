#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include <stdint.h>
#include <stdio.h>
#include "string.h"
#include "n32l43x.h"

// Log levels
typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_VERBOSE,
} boot_log_level_t;

// Default log level, can be overridden by compiler flags
#ifndef BOOT_LOG_LEVEL
#define BOOT_LOG_LEVEL LOG_LEVEL_INFO
#endif

// Log macro
#define BOOT_LOG(level, fmt, ...)                                                   \
    do {                                                                            \
        if (level <= BOOT_LOG_LEVEL) {                                              \
            printf("[%s] " fmt "\r\n",                                              \
                   level == LOG_LEVEL_ERROR ? "E" : level == LOG_LEVEL_WARN  ? "W"  \
                                                : level == LOG_LEVEL_INFO    ? "I"  \
                                                : level == LOG_LEVEL_DEBUG   ? "D"  \
                                                : level == LOG_LEVEL_VERBOSE ? "V"  \
                                                                             : "U", \
                   ##__VA_ARGS__);                                                  \
        }                                                                           \
    } while (0)

#define BOOT_LOG_ERROR(fmt, ...)   BOOT_LOG(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define BOOT_LOG_WARN(fmt, ...)    BOOT_LOG(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define BOOT_LOG_INFO(fmt, ...)    BOOT_LOG(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define BOOT_LOG_DEBUG(fmt, ...)   BOOT_LOG(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define BOOT_LOG_VERBOSE(fmt, ...) BOOT_LOG(LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)

#ifndef APP_START_ADDR
#define APP_START_ADDR 0x08006000
#endif

#ifndef UPDATE_FLAG_ADDR
#define UPDATE_FLAG_ADDR 0x0801F800
#endif

#ifndef UPDATE_FLAG_MASK
#define UPDATE_FLAG_MASK 0x12345678
#endif

// application function
void app_run(uint32_t app_addr);
bool app_is_valid(uint32_t app_addr);

// systimer function
void bootloader_systimer_init(void);
void bootloader_systimer_run_tasks(void);
int  bootloader_systimer_add_task(void (*task_func)(void), uint32_t interval_ms, bool is_periodic);
int  bootloader_systimer_del_task(int task_index);
int  bootloader_systimer_reset_task(int task_index);

// watch dog function
void bootloader_wdt_init(void);
void bootloader_wdt_feed(void);

// dfu function
void bootloader_dfu_init(void);

#endif // __BOOTLOADER_H