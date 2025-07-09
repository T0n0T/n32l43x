#ifndef __CMD_IMPL_H__
#define __CMD_IMPL_H__

#include "cJSON.h"

// 定义与JSON模板对应的结构体
typedef struct {
    int  tick;
    int  dir;
    char model[64]; // 假设model的长度不超过63个字符
} cmd_config_t;

#endif // __CMD_IMPL_H__