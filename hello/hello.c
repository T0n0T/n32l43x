#include "stdint.h"
#include "stdio.h"
#include "hello.h"

__attribute__((weak)) int printf(const char* format, ...);

const char* hello()
{
    printf("I'm amy\r\n");
    return "amy";
}

__attribute__((section(".func_table"))) func_instance func_table[] = {
    {"hello", hello},
};
