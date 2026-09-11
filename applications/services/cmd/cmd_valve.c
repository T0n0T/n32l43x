#include "qpc.h" // QP/C real-time embedded framework
#include "bsp.h" // Board Support Package interface
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "cmd.h"
#include "valve.h"
#include "log.h"

/*
configure the command line interface for the application.
json template:
{
    "count": 2,
    "dir": true,
    "model": "model_name",
}

*/

/* The command gate prevents this event from being reused while it is queued. */
static ValveEvt     evt;
static cmd_config_t config; // 使用静态配置数据
static volatile bool valve_info_response_pending;

static void cmd_valve_info_set_pending(bool pending)
{
    QF_CRIT_ENTRY();
    valve_info_response_pending = pending;
    QF_CRIT_EXIT();
}

static bool cmd_valve_info_take_pending(void)
{
    bool pending;

    QF_CRIT_ENTRY();
    pending = valve_info_response_pending;
    valve_info_response_pending = false;
    QF_CRIT_EXIT();
    return pending;
}

// 解码函数：将JSON字符串解析到cmd_config_t结构体中
int cmd_config_decode(const char* json_string, cmd_config_t* config)
{
    cJSON* root = cJSON_Parse(json_string);
    if (root == NULL) {
        return -1; // 解析失败
    }

    cJSON* tick_item = cJSON_GetObjectItemCaseSensitive(root, "tick");
    if (cJSON_IsNumber(tick_item)) {
        config->tick = tick_item->valueint;
    } else {
        cJSON_Delete(root);
        return -1; // valve_count不存在或类型不正确
    }

    cJSON* dir_item = cJSON_GetObjectItemCaseSensitive(root, "dir");
    if (cJSON_IsBool(dir_item)) {
        config->dir = cJSON_IsTrue(dir_item) ? 1 : -1;
    } else {
        cJSON_Delete(root);
        return -1; // valve_count不存在或类型不正确
    }

    cJSON* model_item = cJSON_GetObjectItemCaseSensitive(root, "model");
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
char* cmd_config_encode(const cmd_config_t* config)
{
    cJSON* root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }

    cJSON_AddNumberToObject(root, "tick", config->tick);
    cJSON_AddStringToObject(root, "model", config->model);
    if (config->dir == 1) {
        cJSON_AddTrueToObject(root, "dir");
    } else if (config->dir == -1) {
        cJSON_AddFalseToObject(root, "dir");
    }
    char* json_string = cJSON_Print(root);
    cJSON_Delete(root);
    return json_string; // 调用者负责释放此字符串
}

void cmd_config_read_wrapper(void* msg)
{
    cmd_config_t* config      = (cmd_config_t*)msg;
    char*         json_string = cmd_config_encode(config);
    APP_LOG_DEBUG("read valve config");
    if (json_string != NULL) {
        size_t  json_len = strlen(json_string);
        uint8_t tx_buf[CMD_TX_BUF_LEN];

        if (json_len > (CMD_TX_BUF_LEN - 2U)) {
            APP_LOG_ERROR("Encoded valve config is too long: %u.", json_len);
            free(json_string);
            cmd_async_complete();
            return;
        }

        memcpy(tx_buf, json_string, json_len);
        tx_buf[json_len]     = '\n';
        tx_buf[json_len + 1] = '\r';
        cmd_dma_transmit(tx_buf, (uint16_t)(json_len + 2U));
        free(json_string);
    } else {
        APP_LOG_ERROR("Error: Failed to encode command configuration.");
    }
    cmd_async_complete();
}

int cmd_config_write(int argc, char** argv)
{
    if (argc != 1) {
        APP_LOG_ERROR("Usage: config_write <json_string>");
        return -1;
    }

    // 测试用的json_string
    // char* json_string = "{\"tick\": 10, \"dir\": false, model\": \"test_model\"}";
    // 实际使用时，请将上一行注释掉，并使用下一行
    char* json_string = argv[0];

    memset(&config, 0, sizeof(cmd_config_t));
    if (cmd_config_decode(json_string, &config) != 0) {
        APP_LOG_ERROR("Error: Failed to decode command configuration.");
        return -1;
    }
    config.flag = FLAG_VAILD;
    QEvt_ctor(&evt.super, VALVE_CONFIG_WRITE_SIG);
    evt.msg     = &config;
    evt.evtType = VALVE_CMD;

    if (!QACTIVE_POST_X(AO_ValveHandler, &evt.super, 1U, 0U)) {
        return -1;
    }

    return 0;
}

int cmd_config_read(int argc, char** argv)
{
    (void)argc; // 未使用参数
    (void)argv; // 未使用参数

    cmd_async_begin();
    QEvt_ctor(&evt.super, VALVE_CONFIG_READ_SIG);
    evt.handle  = cmd_config_read_wrapper;
    evt.evtType = VALVE_CMD;

    if (!QACTIVE_POST_X(AO_ValveHandler, &evt.super, 1U, 0U)) {
        cmd_async_complete();
        return -1;
    }

    return 0;
}

int cmd_config_refactory(int argc, char** argv)
{
    (void)argc; // 未使用参数
    (void)argv; // 未使用参数

    memset(&config, 0, sizeof(cmd_config_t));
    QEvt_ctor(&evt.super, VALVE_REFACTORY_SIG);
    evt.evtType = VALVE_CMD;

    if (!QACTIVE_POST_X(AO_ValveHandler, &evt.super, 1U, 0U)) {
        return -1;
    }
    return 0;
}

void cmd_valve_info_wrapper(void* msg)
{
    cmd_dma_transmit((uint8_t*)msg, sizeof(ValveVal));
}

static void cmd_valve_info_enable_wrapper(void* msg)
{
    cmd_valve_info_wrapper(msg);
    if (cmd_valve_info_take_pending()) {
        cmd_async_complete();
    }
}

static void cmd_valve_info_disable_wrapper(void* msg)
{
    (void)msg;
    cmd_valve_info_set_pending(false);
    cmd_async_complete();
}

void cmd_valve_info_reset(void)
{
    cmd_valve_info_set_pending(false);
}

int cmd_valve_info(int argc, char** argv)
{
    if (argc != 1) {
        APP_LOG_ERROR("Usage: valve_info <0/1>");
        return -1;
    }

    int is_enable = atoi(argv[0]);
    if (is_enable != 0 && is_enable != 1) {
        APP_LOG_ERROR("Error: Invalid argument. Use 0 or 1.");
        return -1;
    }
    APP_LOG_INFO("Valve info command received with is_enable: %d", is_enable);
    cmd_async_begin();
    cmd_valve_info_set_pending(is_enable != 0);
    QEvt_ctor(&evt.super, VALVE_INFO_READ_SIG);
    evt.handle  = is_enable ? cmd_valve_info_enable_wrapper : cmd_valve_info_disable_wrapper;
    evt.msg     = (void*)(intptr_t)is_enable;
    evt.evtType = VALVE_CMD;

    if (!QACTIVE_POST_X(AO_ValveHandler, &evt.super, 1U, 0U)) {
        cmd_valve_info_set_pending(false);
        cmd_async_complete();
        return -1;
    }
    return 0;
}

int cmd_valve_tuning(int argc, char** argv)
{
    if (argc != 1) {
        APP_LOG_ERROR("Usage: valve_tuning <0/1>");
        return -1;
    }
    int is_enable = atoi(argv[0]);
    if (is_enable != 0 && is_enable != 1) {
        APP_LOG_ERROR("Error: Invalid argument. Use 0 or 1.");
        return -1;
    }
    APP_LOG_INFO("Valve tuning command received with is_enable: %d", is_enable);
    if (is_enable) {
        QEvt_ctor(&evt.super, VALVE_TUNING_START_SIG);
        evt.handle = cmd_valve_info_wrapper;
    } else {
        QEvt_ctor(&evt.super, VALVE_TUNING_STOP_SIG);
    }

    evt.evtType = VALVE_CMD;

    if (!QACTIVE_POST_X(AO_ValveHandler, &evt.super, 1U, 0U)) {
        return -1;
    }
    return 0;
}

int cmd_reboot(int argc, char** argv)
{
    (void)argc; // 未使用参数
    (void)argv; // 未使用参数
    APP_LOG_INFO("System is rebooting...");
    QEvt_ctor(&evt.super, VALVE_REBOOT_SIG);
    evt.evtType = VALVE_CMD;
    if (!QACTIVE_POST_X(AO_ValveHandler, &evt.super, 1U, 0U)) {
        return -1;
    }
    return 0;
}

int cmd_update(int argc, char** argv)
{
    (void)argc; // 未使用参数
    (void)argv; // 未使用参数
#ifndef UPDATE_FLAG_MASK
#define UPDATE_FLAG_MASK 0x1234
#endif

    flash_erase_option();
    flash_program_option(UPDATE_FLAG_MASK);

    APP_LOG_INFO("Go to Boot...");
    QEvt_ctor(&evt.super, VALVE_REBOOT_SIG);
    evt.evtType = VALVE_CMD;
    if (!QACTIVE_POST_X(AO_ValveHandler, &evt.super, 1U, 0U)) {
        return -1;
    }
    return 0;
}
