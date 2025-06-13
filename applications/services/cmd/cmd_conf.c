#include "qpc.h" // QP/C real-time embedded framework
#include "bsp.h" // Board Support Package interface
#include "stdio.h"
#include "string.h"
#include "cmd.h"
#include "valve.h"

static ValveEvt     evt;    // 使用静态事件
static cmd_config_t config; // 使用静态变量

/*
configure the command line interface for the application.
json template:
{
    "valve_count": 5,
    "model": "model_name",
}

*/

// 解码函数：将JSON字符串解析到cmd_config_t结构体中
int cmd_config_decode(const char *json_string, cmd_config_t *config) {
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        return -1; // 解析失败
    }

    cJSON *valve_count_item = cJSON_GetObjectItemCaseSensitive(root, "valve_count");
    if (cJSON_IsNumber(valve_count_item)) {
        config->valve_count = valve_count_item->valueint;
    } else {
        cJSON_Delete(root);
        return -1; // valve_count不存在或类型不正确
    }

    cJSON *model_item = cJSON_GetObjectItemCaseSensitive(root, "model");
    if (cJSON_IsString(model_item) && (model_item->valuestring != NULL)) {
        strncpy(config->model, model_item->valuestring, sizeof(config->model) - 1);
        config->model[sizeof(config->model) - 1] = '\0'; // 确保字符串以null结尾
    } else {
        cJSON_Delete(root);
        return -1; // model不存在或类型不正确
    }

    cJSON_Delete(root);
    return 0; // 成功
}

// 编码函数：将cmd_config_t结构体编码成JSON字符串
char* cmd_config_encode(const cmd_config_t *config) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }

    cJSON_AddNumberToObject(root, "valve_count", config->valve_count);
    cJSON_AddStringToObject(root, "model", config->model);

    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);
    return json_string; // 调用者负责释放此字符串
}

int cmd_config_write(int argc, char** argv)
{
    if (argc != 1) {
        printf("Usage: config_write <json_string>\r\n");
        return -1;
    }

    // 测试用的json_string
    char* json_string = "{\"valve_count\": 10, \"model\": \"test_model\"}";
    // 实际使用时，请将上一行注释掉，并使用下一行
    // char* json_string = argv[0];
    
    if (cmd_config_decode(json_string, &config) != 0) {
        printf("Error: Failed to decode command configuration.\r\n");
        return -1;
    }
    QEvt_ctor(&evt.super, VALVE_CONFIG_WRITE_SIG); // 初始化事件
    evt.msg = &config; // 将静态config数据指针赋给事件的msg字段
    evt.evtType = VALVE_CMD; // 设置事件类型

    // 发布事件，生命周期由发送者管理 (非0)
    QACTIVE_POST(AO_ValveHandler, &evt.super, 1U);

    printf("Command configuration written and event posted.\r\n");
    return 0;
}

int cmd_config_read(int argc, char** argv)
{
    (void)argc; // 未使用参数
    (void)argv; // 未使用参数

    QEvt_ctor(&evt.super, VALVE_CONFIG_READ_SIG); // 初始化事件
    evt.msg     = &config;                        // 将静态config数据指针赋给事件的msg字段
    evt.evtType = VALVE_CMD;                      // 设置事件类型

    // 发布事件
    QACTIVE_POST(AO_ValveHandler, &evt.super, 1U);

    printf("Reading config: Request sent.\r\n");
    return 0;
}

int cmd_config_reset(int argc, char** argv)
{
    (void)argc; // 未使用参数
    (void)argv; // 未使用参数

    memset(&config, 0, sizeof(cmd_config_t)); // 重置配置数据
    QEvt_ctor(&evt.super, VALVE_CONFIG_WRITE_SIG); // 初始化事件
    evt.msg     = &config;                         // 将静态config数据指针赋给事件的msg字段
    evt.evtType = VALVE_CMD;                       // 设置事件类型

    QACTIVE_POST(AO_ValveHandler, &evt.super, 1U);
    return 0;
}