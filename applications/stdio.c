#include "stdio.h"
#include "uart.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

// 内部辅助函数声明
static void _putchar(char c);
static int _strlen(const char* str);
static char* _itoa(int value, char* str, int base);
static char* _utoa(unsigned int value, char* str, int base);
static char* _ltoa(long value, char* str, int base);
static char* _ultoa(unsigned long value, char* str, int base);

// 字符输出函数实现
static void _putchar(char c)
{
    uart_putc(CONSOLE, (uint8_t)c);
}

int putchar(int c)
{
    _putchar((char)c);
    return c;
}

int puts(const char* s)
{
    if (!s) return -1;
    
    while (*s) {
        _putchar(*s++);
    }
    _putchar('\n');
    return 0;
}

// 字符串长度计算
static int _strlen(const char* str)
{
    int len = 0;
    if (!str) return 0;
    while (*str++) len++;
    return len;
}

// 整数转字符串函数
static char* _itoa(int value, char* str, int base)
{
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    int tmp_value;
    bool negative = false;

    if (value < 0 && base == 10) {
        negative = true;
        value = -value;
    }

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdef"[tmp_value - value * base];
    } while (value);

    if (negative) *ptr++ = '-';
    *ptr-- = '\0';

    // 反转字符串
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    return str;
}

static char* _utoa(unsigned int value, char* str, int base)
{
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    unsigned int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdef"[tmp_value - value * base];
    } while (value);

    *ptr-- = '\0';

    // 反转字符串
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    return str;
}

static char* _ltoa(long value, char* str, int base)
{
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    long tmp_value;
    bool negative = false;

    if (value < 0 && base == 10) {
        negative = true;
        value = -value;
    }

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdef"[tmp_value - value * base];
    } while (value);

    if (negative) *ptr++ = '-';
    *ptr-- = '\0';

    // 反转字符串
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    return str;
}

static char* _ultoa(unsigned long value, char* str, int base)
{
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    unsigned long tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdef"[tmp_value - value * base];
    } while (value);

    *ptr-- = '\0';

    // 反转字符串
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    return str;
}

// vsnprintf实现 - 核心格式化函数
int vsnprintf(char* str, size_t size, const char* format, va_list args)
{
    char* out = str;
    char* end = str + size - 1;
    char buffer[32];
    
    if (!format || size == 0) return 0;
    
    while (*format && out < end) {
        if (*format != '%') {
            *out++ = *format++;
            continue;
        }
        
        format++; // 跳过 '%'
        
        // 处理格式说明符
        switch (*format) {
            case 'c': {
                char c = (char)va_arg(args, int);
                if (out < end) *out++ = c;
                break;
            }
            case 's': {
                char* s = va_arg(args, char*);
                if (!s) s = "(null)";
                while (*s && out < end) {
                    *out++ = *s++;
                }
                break;
            }
            case 'd':
            case 'i': {
                int val = va_arg(args, int);
                _itoa(val, buffer, 10);
                char* p = buffer;
                while (*p && out < end) {
                    *out++ = *p++;
                }
                break;
            }
            case 'u': {
                unsigned int val = va_arg(args, unsigned int);
                _utoa(val, buffer, 10);
                char* p = buffer;
                while (*p && out < end) {
                    *out++ = *p++;
                }
                break;
            }
            case 'x': {
                unsigned int val = va_arg(args, unsigned int);
                _utoa(val, buffer, 16);
                char* p = buffer;
                while (*p && out < end) {
                    *out++ = *p++;
                }
                break;
            }
            case 'X': {
                unsigned int val = va_arg(args, unsigned int);
                _utoa(val, buffer, 16);
                char* p = buffer;
                while (*p && out < end) {
                    char c = *p++;
                    if (c >= 'a' && c <= 'f') c = c - 'a' + 'A';
                    *out++ = c;
                }
                break;
            }
            case 'l': {
                format++; // 跳过 'l'
                if (*format == 'd' || *format == 'i') {
                    long val = va_arg(args, long);
                    _ltoa(val, buffer, 10);
                    char* p = buffer;
                    while (*p && out < end) {
                        *out++ = *p++;
                    }
                } else if (*format == 'u') {
                    unsigned long val = va_arg(args, unsigned long);
                    _ultoa(val, buffer, 10);
                    char* p = buffer;
                    while (*p && out < end) {
                        *out++ = *p++;
                    }
                } else if (*format == 'x') {
                    unsigned long val = va_arg(args, unsigned long);
                    _ultoa(val, buffer, 16);
                    char* p = buffer;
                    while (*p && out < end) {
                        *out++ = *p++;
                    }
                }
                break;
            }
            case 'p': {
                void* ptr = va_arg(args, void*);
                _ultoa((unsigned long)ptr, buffer, 16);
                if (out < end) *out++ = '0';
                if (out < end) *out++ = 'x';
                char* p = buffer;
                while (*p && out < end) {
                    *out++ = *p++;
                }
                break;
            }
            case '%': {
                if (out < end) *out++ = '%';
                break;
            }
            default: {
                // 未知格式说明符，直接输出
                if (out < end) *out++ = '%';
                if (out < end) *out++ = *format;
                break;
            }
        }
        format++;
    }
    
    *out = '\0';
    return out - str;
}

// sprintf实现
int sprintf(char* str, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vsnprintf(str, SIZE_MAX, format, args);
    va_end(args);
    return result;
}

// snprintf实现
int snprintf(char* str, size_t size, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vsnprintf(str, size, format, args);
    va_end(args);
    return result;
}

// vsprintf实现
int vsprintf(char* str, const char* format, va_list args)
{
    return vsnprintf(str, SIZE_MAX, format, args);
}

// vprintf实现
int vprintf(const char* format, va_list args)
{
    char buffer[256];
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    
    for (int i = 0; i < len; i++) {
        _putchar(buffer[i]);
    }
    
    return len;
}

// printf实现
int printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);
    return result;
}