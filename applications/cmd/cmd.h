#ifndef __CMD_H__
#define __CMD_H__

#define CMD_DEFINE(func)   int cmd_##func(int argc, char** argv);

#define CMD_DEFINE_LIST {    \
    {"reboot", cmd_reboot, 1}, \
    {"update", cmd_update, 1}, \
}

// 命令处理函数类型定义
typedef int (*cmd_handler_t)(int argc, char** argv);

// 命令结构体
typedef struct {
    const char*   name;     // 命令名称
    cmd_handler_t handler;  // 处理函数
    int           max_args; // 最大参数个数
} command_t;

void cmd_init(void);
void cmd_execute(char* input);

CMD_DEFINE(reboot)
CMD_DEFINE(update)

#endif // __CMD_H__