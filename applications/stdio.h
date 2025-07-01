#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 自定义printf函数声明
int printf(const char* format, ...);
int sprintf(char* str, const char* format, ...);
int snprintf(char* str, size_t size, const char* format, ...);
int vprintf(const char* format, va_list args);
int vsprintf(char* str, const char* format, va_list args);
int vsnprintf(char* str, size_t size, const char* format, va_list args);

// 字符输出函数
int putchar(int c);
int puts(const char* s);

#ifdef __cplusplus
}
#endif

#endif /* __STDIO_H__ */