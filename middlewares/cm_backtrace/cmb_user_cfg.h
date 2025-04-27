#ifndef _CMB_USER_CFG_H_
#define _CMB_USER_CFG_H_

#include <stdio.h>

#define cmb_println(...)     \
    printf(__VA_ARGS__); \
    printf("\r\n")

#define CMB_USING_BARE_METAL_PLATFORM
#define CMB_CPU_PLATFORM_TYPE CMB_CPU_ARM_CORTEX_M4
#define CMB_USING_DUMP_STACK_INFO
#define CMB_PRINT_LANGUAGE CMB_PRINT_LANGUAGE_ENGLISH

#endif