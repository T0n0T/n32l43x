#include "at.h"

const at_cmd_t at_lora_cmd[] = {
    {"+++", "OK\r\n", 500},
    {"AT+LPR=5\r\n", "OK\r\n", 500},
    {"AT+JON\r\n", "AT+JON:OK\r\n", 5000},
};
