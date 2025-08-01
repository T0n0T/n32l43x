#ifndef __LOG_H__
#define __LOG_H__



typedef enum {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_VERBOSE,
} app_log_level_t;

// Default log level, can be overridden by compiler flags
#ifndef APP_LOG_LEVEL
#define APP_LOG_LEVEL LOG_LEVEL_DEBUG
#endif

// Log macro
#ifdef DEBUG
#include "SEGGER_RTT.h"
#define APP_LOG_RAW(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#else
#include <stdio.h>
#define APP_LOG_RAW(...) printf(__VA_ARGS__)
#endif

#define APP_LOG(level, fmt, ...)                                                    \
    do {                                                                            \
        if (level <= APP_LOG_LEVEL) {                                               \
            APP_LOG_RAW("[%s] " fmt "\r\n",                                              \
                   level == LOG_LEVEL_ERROR ? "E" : level == LOG_LEVEL_WARN  ? "W"  \
                                                : level == LOG_LEVEL_INFO    ? "I"  \
                                                : level == LOG_LEVEL_DEBUG   ? "D"  \
                                                : level == LOG_LEVEL_VERBOSE ? "V"  \
                                                                             : "U", \
                   ##__VA_ARGS__);                                                  \
        }                                                                           \
    } while (0)

#define APP_LOG_ERROR(fmt, ...)   APP_LOG(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define APP_LOG_WARN(fmt, ...)    APP_LOG(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define APP_LOG_INFO(fmt, ...)    APP_LOG(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define APP_LOG_DEBUG(fmt, ...)   APP_LOG(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define APP_LOG_VERBOSE(fmt, ...) APP_LOG(LOG_LEVEL_VERBOSE, fmt, ##__VA_ARGS__)

#endif