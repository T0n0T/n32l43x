#include "bootloader.h"
#include "uart.h"
#include "flash.h"

#define UART_DFU         USART2
#define UART_DFU_HANDLER USART2_IRQHandler

#define BLE_PWR_PORT     GPIOB
#define BLE_PWR_CLK      RCC_APB2_PERIPH_GPIOB
#define BLE_PWR_PIN      GPIO_PIN_6
#define BLE_PWR_HIGH     BLE_PWR_PORT->PBSC = BLE_PWR_PIN;
#define BLE_PWR_LOW      BLE_PWR_PORT->PBC = BLE_PWR_PIN;

#define ACK_PATTERN      0x12345678 // 示例ACK模式
#define ACK_TIMEOUT_MS   1000       // ACK超时时间（毫秒）
#define MAX_RETRY_COUNT  5          // 最大重试次数

#define DFU_PAGE_LEN     2048
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
    dfu_state state;
    dfu_state target_state;                              // 目标状态
    bool      ack_waiting;                               // 是否在等待ACK
    uint32_t  ack_pattern;                               // ACK模式
    uint32_t  current_block_index;                       // 当前块
    uint32_t  total_block;                               // 总块数
    uint32_t  data_received;                             // 当前块已接收字节数
    uint32_t  flash_base_addr;                           // 固件写入的起始地址（如0x08008000）
    uint32_t  flash_offset;                              // 当前写入偏移
    bool      is_verified;                               // 当前块验签结果
    uint8_t*  public_key;                                // ECDSA公钥(Curve25519)
    uint8_t   header_buf[sizeof(firmware_block_header)]; // 块头缓存
    uint8_t   data_buf[DFU_PAGE_LEN];             // 块数据缓存
} firmware_updater;

#define DFU_RX_BUF_SIZE (DFU_PAGE_LEN > sizeof(firmware_block_header) ? DFU_PAGE_LEN : sizeof(firmware_block_header))
static uint8_t dfu_rx_buf[DFU_RX_BUF_SIZE];
static uint8_t public_key[32] = ED25519_PUBKEY;

static firmware_updater dfu_updater;
static int              dfu_reset_task_index;
static int              dfu_ack_timeout_task_index; // ACK超时任务索引
static uint8_t          dfu_ack_retry_count;        // ACK重试次数

static void bootloader_dfu_preset_state(dfu_state new_state);

// UART 空闲中断处理函数
void USART2_IRQHandler(void)
{
    if (USART_GetIntStatus(UART_DFU, USART_INT_IDLEF) != RESET) {
        (void)UART_DFU->STS;
        (void)UART_DFU->DAT;
        // 停止 DMA 传输
        DMA_EnableChannel(DMA_CH6, DISABLE);

        // 获取当前 DMA 传输的剩余数据量
        uint16_t received_len = DFU_RX_BUF_SIZE - DMA_GetCurrDataCounter(DMA_CH6);

        // 根据 DFU 状态处理接收到的数据
        switch (dfu_updater.state) {
            case DFU_STATE_IDLE: {
                static const uint8_t preamble[]   = DFU_PREAMBLE;
                static uint8_t       preamble_idx = 0;
                if (received_len > 0) {
                    for (uint16_t i = 0; i < received_len; i++) {
                        if (dfu_rx_buf[i] == preamble[preamble_idx]) {
                            preamble_idx++;
                            if (preamble_idx == sizeof(preamble)) {
                                bootloader_dfu_preset_state(DFU_STATE_PREPARE);
                                dfu_updater.data_received = 0;
                                preamble_idx              = 0; // Reset for next potential "Start" if needed
                                break;                         // Found "Start", no need to check further in this packet
                            }
                        } else {
                            preamble_idx = 0; // Reset if mismatch
                        }
                    }
                }
                break;
            }
            case DFU_STATE_PREPARE:
                if (received_len >= 4) {
                    memcpy(&dfu_updater.total_block, dfu_rx_buf, 4);
                    bootloader_dfu_preset_state(DFU_STATE_HEADER);
                }
                break;
            case DFU_STATE_HEADER:
                if (dfu_updater.data_received + received_len <= sizeof(firmware_block_header)) {
                    memcpy(&dfu_updater.header_buf[dfu_updater.data_received], dfu_rx_buf, received_len);
                    dfu_updater.data_received += received_len;
                } else {
                    // 错误处理：接收数据超出块头大小
                    dfu_updater.state = DFU_STATE_ERROR;
                }
                break;
            case DFU_STATE_DATA:
                firmware_block_header* header = (firmware_block_header*)dfu_updater.header_buf;
                if (dfu_updater.data_received + received_len <= header->block_size) {
                    memcpy(&dfu_updater.data_buf[dfu_updater.data_received], dfu_rx_buf, received_len);
                    dfu_updater.data_received += received_len;
                } else {
                    // 错误处理：接收数据超出块头大小
                    dfu_updater.state = DFU_STATE_ERROR;
                }
                break;
            case DFU_SATTE_WAIT_ACK:
                if (received_len >= sizeof(uint32_t)) {
                    uint32_t received_ack;
                    memcpy(&received_ack, dfu_rx_buf, sizeof(uint32_t));
                    if (received_ack == dfu_updater.ack_pattern) {
                        BOOT_LOG_DEBUG("ACK received for state %d", dfu_updater.target_state);
                        bootloader_systimer_del_task(dfu_ack_timeout_task_index); // 取消超时任务
                        dfu_updater.state       = dfu_updater.target_state;
                        dfu_updater.ack_waiting = false;
                    }
                }
                break;
            default:
                break;
        }

        // 重新配置 DMA 并启动 DMA 接收
        DMA_SetCurrDataCounter(DMA_CH6, DFU_RX_BUF_SIZE);
        DMA_EnableChannel(DMA_CH6, ENABLE);
    }
    if ((USART_GetFlagStatus(UART_DFU, USART_FLAG_OREF) != RESET) ||
        (USART_GetFlagStatus(UART_DFU, USART_FLAG_NEF) != RESET) ||
        (USART_GetFlagStatus(UART_DFU, USART_FLAG_PEF) != RESET) ||
        (USART_GetFlagStatus(UART_DFU, USART_FLAG_FEF) != RESET)) {
        /*Read the sts register first,and the read the DAT register to clear the all error flag*/
        (void)UART_DFU->STS;
        (void)UART_DFU->DAT;
        /* Under normal circumstances, all error flags will be cleared when the upper data is read and will not be executed here;
           users can add their own processing according to the actual scenario. */
    }
}

static void bootloader_dfu_reset(void)
{
    BOOT_LOG_WARN("long timer no byte,reset\r\n");
#ifdef DEBUG /* debug build? */
    cm_backtrace_assert(cmb_get_sp());
    while (1); /* tie the CPU in this endless loop */
#endif
    NVIC_SystemReset(); /* reset the CPU */
}

static void bootloader_dfu_serial_init(void)
{
    GPIO_InitType GPIO_InitStructure;
    GPIO_InitStruct(&GPIO_InitStructure);
    RCC_EnableAPB2PeriphClk(BLE_PWR_CLK, ENABLE);
    GPIO_InitStructure.Pin       = BLE_PWR_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitPeripheral(BLE_PWR_PORT, &GPIO_InitStructure);

    BLE_PWR_HIGH;

    uart_init(BLE_SERIAL);
    USART_Enable(UART_DFU, DISABLE);

    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA, ENABLE);
    DMA_InitType DMA_InitStructure;
    DMA_DeInit(DMA_CH6);
    DMA_InitStructure.PeriphAddr     = ((uint32_t)UART_DFU + 0x04);
    DMA_InitStructure.MemAddr        = (uint32_t)dfu_rx_buf;
    DMA_InitStructure.Direction      = DMA_DIR_PERIPH_SRC;
    DMA_InitStructure.BufSize        = DFU_RX_BUF_SIZE;
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.CircularMode   = DMA_MODE_NORMAL; // 修改为普通模式
    DMA_InitStructure.Priority       = DMA_PRIORITY_VERY_HIGH;
    DMA_InitStructure.Mem2Mem        = DMA_M2M_DISABLE;
    DMA_Init(DMA_CH6, &DMA_InitStructure);
    DMA_RequestRemap(DMA_REMAP_USART2_RX, DMA, DMA_CH6, ENABLE);
    USART_EnableDMA(UART_DFU, USART_DMAREQ_RX, ENABLE);
    DMA_EnableChannel(DMA_CH6, ENABLE);
    USART_Enable(UART_DFU, ENABLE);

    // 启用 UART 空闲中断
    USART_ConfigInt(UART_DFU, USART_INT_IDLEF, ENABLE);

    // 启用 UART 中断
    NVIC_SetPriority(USART2_IRQn, 0U);
    NVIC_EnableIRQ(USART2_IRQn);
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

static void bootloader_dfu_ack_timeout(void)
{
    if (dfu_updater.state == DFU_SATTE_WAIT_ACK && dfu_ack_retry_count < MAX_RETRY_COUNT - 1) {
        dfu_ack_retry_count++;
        BOOT_LOG_WARN("ACK %s timeout, retrying %d/%d", dfu_state_to_string(dfu_updater.target_state), dfu_ack_retry_count, MAX_RETRY_COUNT);
        USART_SendData(UART_DFU, (uint16_t)dfu_updater.target_state);
        while (USART_GetFlagStatus(UART_DFU, USART_FLAG_TXC) == RESET);
    } else {
        BOOT_LOG_ERROR("ACK timeout exceeded max retries, entering error state.");
        dfu_updater.state = DFU_STATE_ERROR;
    }
}

static void bootloader_dfu_preset_state(dfu_state new_state)
{
    dfu_updater.target_state = new_state;
    dfu_updater.state        = DFU_SATTE_WAIT_ACK;
    dfu_ack_retry_count      = 0;
}

void bootloader_dfu_process(void)
{
    static dfu_state       last_state = DFU_STATE_IDLE;
    firmware_block_header* header     = (firmware_block_header*)dfu_updater.header_buf;

    switch (dfu_updater.state) {
        case DFU_STATE_PREPARE:
            if (dfu_reset_task_index == -1) {
                dfu_reset_task_index = bootloader_systimer_add_task(bootloader_dfu_reset, 10000, false);
            }
            break;
        case DFU_STATE_HEADER:
            if (dfu_updater.data_received >= sizeof(firmware_block_header)) {
                dfu_updater.data_received = 0;
                bootloader_dfu_preset_state(DFU_STATE_DATA);
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
        case DFU_STATE_WRITE:
            uint32_t write_len = header->block_size < DFU_PAGE_LEN ? header->block_size : DFU_PAGE_LEN;
            flash_erase_page(dfu_updater.flash_base_addr + dfu_updater.flash_offset);
            for (uint32_t i = 0; i < write_len; i = i + 4) {
                uint32_t word_data = dfu_updater.data_buf[i] & 0xFF |
                                     (dfu_updater.data_buf[i + 1] & 0xFF) << 8 |
                                     (dfu_updater.data_buf[i + 2] & 0xFF) << 16 |
                                     (dfu_updater.data_buf[i + 3] & 0xFF) << 24;
                flash_program_word(dfu_updater.flash_base_addr + dfu_updater.flash_offset + i,
                                   word_data);
            }
            dfu_updater.flash_offset += DFU_PAGE_LEN;
            BOOT_LOG_INFO("Wrote block %d/%d to flash at address 0x%08X",
                          dfu_updater.current_block_index + 1, dfu_updater.total_block,
                          dfu_updater.flash_base_addr + dfu_updater.flash_offset - DFU_PAGE_LEN);
            bootloader_dfu_preset_state(DFU_STATE_HEADER); // next header
            memset(header, 0, sizeof(firmware_block_header));
            if (++dfu_updater.current_block_index == dfu_updater.total_block) {
                bootloader_dfu_preset_state(DFU_STATE_FINAL);
            }
            break;
        case DFU_SATTE_WAIT_ACK:
            if (!dfu_updater.ack_waiting) {
                dfu_updater.ack_waiting = true;
                BOOT_LOG_VERBOSE("DFU state change requested to %d, entering WAIT_ACK state.", dfu_updater.target_state);
                USART_SendData(UART_DFU, (uint16_t)dfu_updater.target_state);
                while (USART_GetFlagStatus(UART_DFU, USART_FLAG_TXC) == RESET);
                dfu_ack_timeout_task_index = bootloader_systimer_add_task(bootloader_dfu_ack_timeout, ACK_TIMEOUT_MS, true);
            }

            break;
        case DFU_STATE_FINAL:
            BOOT_LOG_INFO("DFU completed, rebooting to application...");
            flash_program_word(UPDATE_FLAG_ADDR, APP_FLAG_MASK);
#ifdef DEBUG /* debug build? */
            cm_backtrace_assert(cmb_get_sp());
            while (1); /* tie the CPU in this endless loop */
#endif
            NVIC_SystemReset(); /* reset the CPU */
            break;
        case DFU_STATE_ERROR:
            BOOT_LOG_ERROR("DFU process encountered an error, resetting...");
            USART_SendData(UART_DFU, DFU_STATE_ERROR);
            while (USART_GetFlagStatus(UART_DFU, USART_FLAG_TXC) == RESET);
#ifdef DEBUG /* debug build? */
            cm_backtrace_assert(cmb_get_sp());
            while (1); /* tie the CPU in this endless loop */
#endif
            NVIC_SystemReset(); /* reset the CPU */
            break;
        default:
            break;
    }
    if (last_state != dfu_updater.state && dfu_updater.state != DFU_SATTE_WAIT_ACK) {
        BOOT_LOG_VERBOSE("DFU state %d --> %d", last_state, dfu_updater.state);
        bootloader_systimer_reset_task(dfu_reset_task_index);
        last_state = dfu_updater.state;
    }
}

void bootloader_dfu_init(void)
{
    if (*(uint32_t*)UPDATE_FLAG_ADDR == UPDATE_FLAG_MASK) {
        flash_erase_page(UPDATE_FLAG_ADDR);
    }
    dfu_updater.state           = DFU_STATE_IDLE;
    dfu_updater.flash_base_addr = APP_START_ADDR;
    dfu_updater.public_key      = public_key;
    dfu_updater.ack_pattern     = ACK_PATTERN; // 初始化ACK模式
    dfu_reset_task_index        = -1;
    dfu_ack_timeout_task_index  = -1;
    dfu_ack_retry_count         = 0; // 初始化重试次数
    bootloader_dfu_serial_init();
    bootloader_systimer_add_task(bootloader_dfu_process, 5, true);
}
