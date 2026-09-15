#include "bootloader.h"
#include "bootble.h"
#include "uart2_dma.h"

#if defined(__arm__) || defined(__thumb__)
// Cortex-M4 内联汇编实现的 memcpy
void* memcpy(void* __restrict dest, const void* __restrict src, size_t n)
{
    unsigned char*       d = dest;
    const unsigned char* s = src;
    asm volatile(
        "cmp %[n], #0\n"  // 比较 n 和 0
        "beq 9f\n"        // 如果 n 为 0，则跳转到结束
        "cmp %[n], #32\n" // 比较 n 和 32
        "blt 2f\n"        // 如果 n < 32，则跳转到字节复制
        "1:\n"
        "ldmia %[s]!, {r0-r7}\n" // 从 src 加载 8 个字 (32 字节)
        "stmia %[d]!, {r0-r7}\n" // 将 8 个字存储到 dest
        "subs %[n], #32\n"       // n 减去 32
        "cmp %[n], #32\n"
        "bge 1b\n" // 如果 n >= 32，则继续循环
        "2:\n"
        "cmp %[n], #0\n" // 检查是否还有剩余字节
        "beq 9f\n"       // 如果 n 为 0，则跳转到结束
        "3:\n"
        "ldrb r0, [%[s]], #1\n" // 加载一个字节
        "strb r0, [%[d]], #1\n" // 存储一个字节
        "subs %[n], #1\n"       // n 减去 1
        "bne 3b\n"              // 如果 n 不为 0，则继续循环
        "9:\n"
        : [d] "+r"(d), [s] "+r"(s), [n] "+r"(n)
        :
        : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "memory", "cc");
    return dest;
}

// Cortex-M4 内联汇编实现的 memset
void* memset(void* s, int c, size_t n)
{
    unsigned char* p   = s;
    unsigned char  val = (unsigned char)c;
    asm volatile(
        "cmp %[n], #0\n"            // 比较 n 和 0
        "beq 9f\n"                  // 如果 n 为 0，则跳转到结束
        "cmp %[n], #32\n"           // 比较 n 和 32
        "blt 2f\n"                  // 如果 n < 32，则跳转到字节填充
        "mov r0, %[val]\n"          // 将 val 移动到 r0
        "orr r0, r0, r0, lsl #8\n"  // 复制 val 到 r0 的高 8 位
        "orr r0, r0, r0, lsl #16\n" // 复制 val 到 r0 的高 16 位 (形成 0xVVVVVVVV)
        "mov r1, r0\n"
        "mov r2, r0\n"
        "mov r3, r0\n"
        "mov r4, r0\n"
        "mov r5, r0\n"
        "mov r6, r0\n"
        "mov r7, r0\n"
        "1:\n"
        "stmia %[p]!, {r0-r7}\n" // 存储 8 个字 (32 字节)
        "subs %[n], #32\n"       // n 减去 32
        "cmp %[n], #32\n"
        "bge 1b\n" // 如果 n >= 32，则继续循环
        "2:\n"
        "cmp %[n], #0\n" // 检查是否还有剩余字节
        "beq 9f\n"       // 如果 n 为 0，则跳转到结束
        "3:\n"
        "strb %[val], [%[p]], #1\n" // 存储一个字节
        "subs %[n], #1\n"           // n 减去 1
        "bne 3b\n"                  // 如果 n 不为 0，则继续循环
        "9:\n"
        : [p] "+r"(p), [n] "+r"(n)
        : [val] "r"(val)
        : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "memory", "cc");
    return s;
}

char* strncpy(char* dest, const char* src, size_t n)
{
    if (dest == NULL || src == NULL || n == 0) {
        return dest;
    }

    char* original_dest = dest;
    while (n > 0 && *src != '\0') {
        *dest++ = *src++;
        n--;
    }

    while (n > 0) {
        *dest++ = '\0';
        n--;
    }

    return original_dest;
}

#endif

#define ACK_PATTERN      0x12345678 // 示例ACK模式
#define ACK_TIMEOUT_MS   1000       // ACK超时时间（毫秒）
#define MAX_RETRY_COUNT  5          // 最大重试次数
#define DFU_STATE_TIMEOUT_MS 60000U
#define BLE_BAUDRATE     115200U

#define DFU_PAGE_LEN     2048
#define DFU_FLASH_END_ADDR 0x08020000U /* End of the target's 128 KiB flash. */
#define DFU_MAX_BLOCKS   ((DFU_FLASH_END_ADDR - APP_START_ADDR) / DFU_PAGE_LEN)
#define DFU_PREAMBLE     {0xAA, 0x55, 0xAA, 0x55}
#define ED25519_PUBKEY   {  \
    0x42, 0xE6, 0x9B, 0x3A, \
    0xEA, 0xE5, 0x0E, 0x7A, \
    0x6C, 0xA9, 0x19, 0xAF, \
    0x3C, 0xAA, 0xBF, 0x1F, \
    0x78, 0xD6, 0x2E, 0x9F, \
    0x52, 0xBC, 0x7C, 0xBE, \
    0x7A, 0x84, 0x38, 0x6E, \
    0xD8, 0x10, 0x9A, 0xAC}

typedef enum {
    DFU_STATE_IDLE,     // 空闲状态，等待开始
    DFU_STATE_PREPARE,  // 准备阶段
    DFU_STATE_HEADER,   // 接收块头
    DFU_STATE_DATA,     // 接收块数据
    DFU_STATE_VERIFY,   // 验签
    DFU_STATE_WRITE,    // 写入Flash
    DFU_STATE_FINAL,    // DFU结束
    DFU_SATTE_WAIT_ACK, // 等待ACK
    DFU_STATE_ERROR     // 错误状态
} dfu_state;

typedef struct {
    uint8_t  signature[64]; // 当前块的 Curve25519 签名（示例为64字节）
    uint32_t block_size;    // 当前块实际大小（字节数，≤分块最大值）
} firmware_block_header;

typedef struct {
    volatile dfu_state state;
    volatile dfu_state target_state;                     // 目标状态
    volatile bool ack_waiting;                           // 是否在等待ACK
    uint32_t  ack_pattern;                               // ACK模式
    uint32_t  current_block_index;                       // 当前块
    uint32_t  total_block;                               // 总块数
    volatile uint32_t data_received;                     // 当前块已接收字节数
    uint32_t  flash_base_addr;                           // 固件写入的起始地址（如0x08008000）
    uint32_t  flash_offset;                              // 当前写入偏移
    bool      is_verified;                               // 当前块验签结果
    uint8_t*  public_key;                                // ECDSA公钥(Curve25519)
    uint8_t   header_buf[sizeof(firmware_block_header)]; // 块头缓存
    uint8_t   data_buf[DFU_PAGE_LEN];                    // 块数据缓存
} firmware_updater;

static uint8_t public_key[32] = ED25519_PUBKEY;

static firmware_updater dfu_updater;
static int              dfu_task_index = -1;
static uint8_t          dfu_ack_retry_count;
static uint8_t          dfu_preamble_idx;
static volatile uint32_t dfu_state_started_ms;
static uint32_t         dfu_ack_sent_ms;

static void bootloader_dfu_preset_state(dfu_state new_state);

static void bootloader_dfu_set_state(dfu_state new_state)
{
    dfu_state_started_ms = bootloader_systimer_millis();
    dfu_updater.state = new_state;
}

static void bootloader_dfu_receive(const uint8_t* data, uint16_t received_len, bool rx_error)
{
    static const uint8_t preamble[] = DFU_PREAMBLE;
    if (rx_error) {
        dfu_preamble_idx = 0;
        if (dfu_updater.state != DFU_STATE_IDLE) {
            dfu_updater.state = DFU_STATE_ERROR;
        }
        return;
    }
    if (received_len == 0U) {
        return;
    }
    switch (dfu_updater.state) {
        case DFU_STATE_IDLE:
            for (uint16_t i = 0; i < received_len; i++) {
                if (data[i] == preamble[dfu_preamble_idx]) {
                    dfu_preamble_idx++;
                    if (dfu_preamble_idx == sizeof(preamble)) {
                        bootloader_dfu_preset_state(DFU_STATE_PREPARE);
                        dfu_updater.data_received = 0;
                        dfu_preamble_idx = 0;
                        break;
                    }
                } else {
                    dfu_preamble_idx = (data[i] == preamble[0]) ? 1 : 0;
                }
            }
            break;
        case DFU_STATE_PREPARE:
            if (received_len >= 4) {
                memcpy(&dfu_updater.total_block, data, 4);
                if (dfu_updater.total_block == 0U || dfu_updater.total_block > DFU_MAX_BLOCKS) {
                    dfu_updater.state = DFU_STATE_ERROR;
                } else {
                    bootloader_dfu_preset_state(DFU_STATE_HEADER);
                }
            }
            break;
        case DFU_STATE_HEADER:
            if (dfu_updater.data_received + received_len <= sizeof(firmware_block_header)) {
                memcpy(&dfu_updater.header_buf[dfu_updater.data_received], data, received_len);
                dfu_updater.data_received += received_len;
            } else {
                dfu_updater.state = DFU_STATE_ERROR;
            }
            break;
        case DFU_STATE_DATA: {
            firmware_block_header* header = (firmware_block_header*)dfu_updater.header_buf;
            if (header->block_size > 0U && header->block_size <= DFU_PAGE_LEN &&
                dfu_updater.data_received + received_len <= header->block_size) {
                memcpy(&dfu_updater.data_buf[dfu_updater.data_received], data, received_len);
                dfu_updater.data_received += received_len;
            } else {
                dfu_updater.state = DFU_STATE_ERROR;
            }
            break;
        }
        case DFU_SATTE_WAIT_ACK:
            if (received_len >= sizeof(uint32_t)) {
                uint32_t received_ack;
                memcpy(&received_ack, data, sizeof(uint32_t));
                if (received_ack == dfu_updater.ack_pattern && dfu_updater.ack_waiting) {
                    BOOT_LOG_DEBUG("ACK received for state %d", dfu_updater.target_state);
                    dfu_updater.ack_waiting = false;
                    bootloader_dfu_set_state(dfu_updater.target_state);
                }
            }
            break;
        default:
            break;
    }
}

static void bootloader_dfu_reset(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    memset(&dfu_updater, 0, sizeof(dfu_updater));
    dfu_updater.flash_base_addr = APP_START_ADDR;
    dfu_updater.public_key = public_key;
    dfu_updater.ack_pattern = ACK_PATTERN;
    dfu_ack_retry_count = 0;
    dfu_preamble_idx = 0;
    dfu_ack_sent_ms = 0;
    bootloader_dfu_set_state(DFU_STATE_IDLE);
    /* Discard DMA bytes from the abandoned transfer before accepting a new one. */
    if (uart2_dma_get_baudrate() != 0U) {
        uart2_dma_set_rx_handler(bootloader_dfu_receive);
    }
    __set_PRIMASK(primask);
}

static const char* dfu_state_to_string(dfu_state state)
{
    switch (state) {
        case DFU_STATE_IDLE:
            return "IDLE";
        case DFU_STATE_PREPARE:
            return "PREPARE";
        case DFU_STATE_HEADER:
            return "HEADER";
        case DFU_STATE_DATA:
            return "DATA";
        case DFU_STATE_VERIFY:
            return "VERIFY";
        case DFU_STATE_WRITE:
            return "WRITE";
        case DFU_STATE_FINAL:
            return "FINAL";
        case DFU_SATTE_WAIT_ACK:
            return "WAIT_ACK";
        case DFU_STATE_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

static void bootloader_dfu_preset_state(dfu_state new_state)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (dfu_updater.state == DFU_STATE_ERROR) {
        __set_PRIMASK(primask);
        return;
    }
    dfu_updater.target_state = new_state;
    dfu_updater.ack_waiting = false;
    dfu_ack_retry_count = 0;
    bootloader_dfu_set_state(DFU_SATTE_WAIT_ACK);
    __set_PRIMASK(primask);
}

static void bootloader_dfu_process(void)
{
    static dfu_state       last_state = DFU_STATE_IDLE;
    firmware_block_header* header     = (firmware_block_header*)dfu_updater.header_buf;

    dfu_state timed_out_state = DFU_STATE_IDLE;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (dfu_updater.state != DFU_STATE_IDLE && dfu_updater.state != DFU_STATE_ERROR &&
        (uint32_t)(bootloader_systimer_millis() - dfu_state_started_ms) >= DFU_STATE_TIMEOUT_MS) {
        timed_out_state = dfu_updater.state;
        dfu_updater.state = DFU_STATE_ERROR;
    }
    __set_PRIMASK(primask);
    if (timed_out_state != DFU_STATE_IDLE) {
        BOOT_LOG_WARN("DFU %s timeout; abandoning the current transfer",
                      dfu_state_to_string(timed_out_state));
    }

    switch (dfu_updater.state) {
        case DFU_STATE_HEADER:
            if (dfu_updater.data_received >= sizeof(firmware_block_header)) {
                if (header->block_size == 0U || header->block_size > DFU_PAGE_LEN) {
                    dfu_updater.state = DFU_STATE_ERROR;
                } else {
                    dfu_updater.data_received = 0;
                    bootloader_dfu_preset_state(DFU_STATE_DATA);
                }
            }
            break;
        case DFU_STATE_DATA:
            if (dfu_updater.data_received >= header->block_size) {
                dfu_updater.data_received = 0;
                bootloader_dfu_preset_state(DFU_STATE_VERIFY);
            }
            break;
        case DFU_STATE_VERIFY:
            // TODO:do verify
            dfu_updater.is_verified = true;
            if (!dfu_updater.is_verified) {
                dfu_updater.state = DFU_STATE_ERROR;
            } else {
                bootloader_dfu_preset_state(DFU_STATE_WRITE);
            }
            break;
        case DFU_STATE_WRITE: {
            uint32_t write_len = header->block_size;
            if (write_len == 0U || write_len > DFU_PAGE_LEN ||
                dfu_updater.current_block_index >= dfu_updater.total_block ||
                dfu_updater.flash_offset >= DFU_FLASH_END_ADDR - APP_START_ADDR) {
                dfu_updater.state = DFU_STATE_ERROR;
                break;
            }
            flash_erase_page(dfu_updater.flash_base_addr + dfu_updater.flash_offset);
            for (uint32_t i = 0; i < write_len; i = i + 4) {
                uint32_t word_data = UINT32_MAX;
                uint32_t bytes = write_len - i < 4U ? write_len - i : 4U;
                memcpy(&word_data, &dfu_updater.data_buf[i], bytes);
                flash_program_word(dfu_updater.flash_base_addr + dfu_updater.flash_offset + i,
                                   word_data);
            }
            dfu_updater.flash_offset += DFU_PAGE_LEN;
            BOOT_LOG_INFO("Wrote block %d/%d to flash at address 0x%X",
                          dfu_updater.current_block_index + 1, dfu_updater.total_block,
                          dfu_updater.flash_base_addr + dfu_updater.flash_offset - DFU_PAGE_LEN);
            bootloader_dfu_preset_state(DFU_STATE_HEADER); // next header
            memset(header, 0, sizeof(firmware_block_header));
            if (++dfu_updater.current_block_index == dfu_updater.total_block) {
                bootloader_dfu_preset_state(DFU_STATE_FINAL);
            }
            break;
        }
        case DFU_SATTE_WAIT_ACK: {
            uint32_t now_ms = bootloader_systimer_millis();
            uint8_t target_state = 0;
            bool send_state = false;
            primask = __get_PRIMASK();
            __disable_irq();
            if (dfu_updater.state == DFU_SATTE_WAIT_ACK && !dfu_updater.ack_waiting) {
                dfu_updater.ack_waiting = true;
                dfu_ack_sent_ms = now_ms;
                send_state = true;
            } else if (dfu_updater.state == DFU_SATTE_WAIT_ACK &&
                       (uint32_t)(now_ms - dfu_ack_sent_ms) >= ACK_TIMEOUT_MS) {
                if (dfu_ack_retry_count < MAX_RETRY_COUNT - 1) {
                    dfu_ack_retry_count++;
                    dfu_ack_sent_ms = now_ms;
                    send_state = true;
                } else {
                    dfu_updater.state = DFU_STATE_ERROR;
                }
            }
            target_state = dfu_updater.target_state;
            __set_PRIMASK(primask);
            if (send_state) {
                BOOT_LOG_VERBOSE("DFU requesting %s, retry %u", dfu_state_to_string(target_state),
                                 (unsigned)dfu_ack_retry_count);
                uart2_dma_write(&target_state, 1);
            }
            break;
        }
        case DFU_STATE_FINAL:
            BOOT_LOG_INFO("DFU completed, rebooting to application...");
            flash_program_option(APP_FLAG_MASK);

            NVIC_SystemReset(); /* reset the CPU */
            break;
        case DFU_STATE_ERROR:
            BOOT_LOG_ERROR("DFU transfer aborted; ready for a new transfer");
            uint8_t error_state = DFU_STATE_ERROR;
            uart2_dma_write(&error_state, 1);
            bootloader_dfu_reset();
            break;
        default:
            break;
    }
    if (last_state != dfu_updater.state && dfu_updater.state != DFU_SATTE_WAIT_ACK) {
        BOOT_LOG_VERBOSE("DFU state %d --> %d", last_state, dfu_updater.state);
        last_state = dfu_updater.state;
    }
}

void bootloader_dfu_init(void)
{
    /* Keep one task across session resets; reserve it before accepting input. */
    if (dfu_task_index < 0) {
        dfu_task_index = bootloader_systimer_add_task(bootloader_dfu_process, 5, true);
        if (dfu_task_index < 0) {
            BOOT_LOG_ERROR("Cannot register DFU task; rebooting");
            NVIC_SystemReset();
            return;
        }
    }
    bootloader_dfu_reset();
    /* On an update reset no BLE handshake is needed. Arm the DFU receiver
     * before the host sends its preamble, then power the module. */
    if (uart2_dma_get_baudrate() == 0U) {
        uart2_dma_init(BLE_BAUDRATE, bootloader_dfu_receive);
        bootloader_ble_power_on();
    }
    if (flash_option_get() == UPDATE_FLAG_MASK) {
        flash_erase_option();
    }
}
