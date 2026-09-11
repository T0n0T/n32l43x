#include "bootble.h"
#include "bootloader.h"
#include "uart2_dma.h"

#define BLE_BAUDRATE          115200U
#define BLE_FALLBACK_BAUDRATE 9600U
#define BLE_START_TIMEOUT_MS  1000U
#define BLE_AT_TIMEOUT_MS     500U
#define BLE_RESET_TIMEOUT_MS  1500U

typedef enum {
    BLE_WAIT_START,
    BLE_PROBE_115200,
    BLE_PROBE_9600,
    BLE_WAIT_BAUD,
    BLE_WAIT_RESET,
    BLE_VERIFY_115200,
    BLE_READY,
    BLE_FAILED,
    BLE_OFF,
} ble_state_t;

static ble_state_t   state;
static uint32_t      state_started_ms;
static volatile bool start_received;
static volatile bool ok_received;
static volatile bool idle_received;
static uint8_t       start_index;
static uint8_t       ok_index;

static bool match_byte(uint8_t byte, const char* pattern, uint8_t* index)
{
    if (byte == (uint8_t)pattern[*index]) {
        (*index)++;
        if (pattern[*index] == '\0') {
            *index = 0;
            return true;
        }
    } else {
        *index = (byte == (uint8_t)pattern[0]) ? 1 : 0;
    }
    return false;
}

static void bootloader_ble_receive(const uint8_t* data, uint16_t len, bool rx_error)
{
    idle_received = true;
    if (rx_error) {
        start_index = 0;
        ok_index    = 0;
        return;
    }
    for (uint16_t i = 0; i < len; i++) {
        if (match_byte(data[i], "Start", &start_index) &&
            uart2_dma_get_baudrate() == BLE_BAUDRATE) {
            start_received = true;
        }
        if (match_byte(data[i], "OK", &ok_index)) {
            ok_received = true;
        }
    }
}

static void begin_command(ble_state_t next, const char* command, uint16_t len, uint32_t now_ms)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    state            = next;
    state_started_ms = now_ms;
    ok_received      = false;
    ok_index         = 0;
    /* A fragmented Start may straddle a probe timeout at the same baud. */
    if (next != BLE_PROBE_115200 && next != BLE_VERIFY_115200) {
        start_received = false;
        start_index    = 0;
    }
    __set_PRIMASK(primask);
    uart2_dma_write((const uint8_t*)command, len);
}

static void bootloader_ble_fail(void)
{
    BOOT_LOG_WARN("BLE startup failed at step %d", state);
    uart2_dma_set_baudrate(BLE_BAUDRATE);
    state = BLE_FAILED;
}

void bootloader_ble_init(uint32_t now_ms)
{
    state            = BLE_WAIT_START;
    state_started_ms = now_ms;
    start_received   = false;
    ok_received      = false;
    idle_received    = false;
    start_index      = 0;
    ok_index         = 0;

    /* Arm RX before powering the module so its first Start can be captured. */
    uart2_dma_init(BLE_BAUDRATE, bootloader_ble_receive);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOB, ENABLE);
    GPIO_InitType config;
    GPIO_InitStruct(&config);
    config.Pin       = GPIO_PIN_6;
    config.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitPeripheral(GPIOB, &config);
    GPIOB->PBSC = GPIO_PIN_6;
}

void bootloader_ble_process(uint32_t now_ms)
{
    uint32_t elapsed_ms = now_ms - state_started_ms;
    switch (state) {
        case BLE_WAIT_START:
        case BLE_PROBE_115200:
        case BLE_VERIFY_115200:
            if (start_received || (state != BLE_WAIT_START && ok_received)) {
                state = BLE_READY;
            } else if (state == BLE_WAIT_START && elapsed_ms >= BLE_START_TIMEOUT_MS) {
                begin_command(BLE_PROBE_115200, "AT", 2, now_ms);
            } else if (state == BLE_PROBE_115200 && elapsed_ms >= BLE_AT_TIMEOUT_MS) {
                BOOT_LOG_INFO("BLE trying 9600 baud (idle=%d)", idle_received);
                uart2_dma_set_baudrate(BLE_FALLBACK_BAUDRATE);
                begin_command(BLE_PROBE_9600, "AT", 2, now_ms);
            } else if (state == BLE_VERIFY_115200 && elapsed_ms >= BLE_AT_TIMEOUT_MS) {
                bootloader_ble_fail();
            }
            break;
        case BLE_PROBE_9600:
            if (ok_received) {
                begin_command(BLE_WAIT_BAUD, "AT+BAUD4", 8, now_ms);
            } else if (elapsed_ms >= BLE_AT_TIMEOUT_MS) {
                bootloader_ble_fail();
            }
            break;
        case BLE_WAIT_BAUD:
            if (ok_received) {
                /* BAUD4 takes effect after RESET. Send RESET fully at 9600,
                 * then arm 115200 RX before the module's next Start. */
                begin_command(BLE_WAIT_RESET, "AT+RESET", 8, now_ms);
                uart2_dma_set_baudrate(BLE_BAUDRATE);
            } else if (elapsed_ms >= BLE_AT_TIMEOUT_MS) {
                bootloader_ble_fail();
            }
            break;
        case BLE_WAIT_RESET:
            if (start_received) {
                state = BLE_READY;
            } else if (elapsed_ms >= BLE_RESET_TIMEOUT_MS) {
                begin_command(BLE_VERIFY_115200, "AT", 2, now_ms);
            }
            break;
        default:
            break;
    }
}

void bootloader_ble_deinit(void)
{
    uart2_dma_deinit();
    GPIOB->PBC = GPIO_PIN_6;
    state      = BLE_OFF;
}

bool bootloader_ble_is_finished(void)
{
    return state == BLE_READY || state == BLE_FAILED || state == BLE_OFF;
}

bool bootloader_ble_is_ready(void)
{
    return state == BLE_READY;
}
