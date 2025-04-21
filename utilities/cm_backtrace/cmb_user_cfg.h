#ifndef _CMB_USER_CFG_H_
#define _CMB_USER_CFG_H_

#define cmb_println(...)     \
    rt_kprintf(__VA_ARGS__); \
    rt_kprintf("\r\n")

#define CMB_USING_OS_PLATFORM
#define CMB_OS_PLATFORM_TYPE  CMB_OS_PLATFORM_RTT
#define CMB_CPU_PLATFORM_TYPE CMB_CPU_ARM_CORTEX_M4
#define CMB_USING_DUMP_STACK_INFO
#define CMB_PRINT_LANGUAGE CMB_PRINT_LANGUAGE_ENGLISH

#endif