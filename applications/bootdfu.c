#include "bootloader.h"
#include "string.h"
#include "uart.h"
#include "flash.h"

#define UART_DFU       USART2
#define DFU_PAGE_LEN   2048
#define DFU_PREAMBLE   {0xAA, 0x55, 0xAA, 0x55}
#define ED25519_PUBKEY {    \
    0x42, 0xE6, 0x9B, 0x3A, \
    0xEA, 0xE5, 0x0E, 0x7A, \
    0x6C, 0xA9, 0x19, 0xAF, \
    0x3C, 0xAA, 0xBF, 0x1F, \
    0x78, 0xD6, 0x2E, 0x9F, \
    0x52, 0xBC, 0x7C, 0xBE, \
    0x7A, 0x84, 0x38, 0x6E, \
    0xD8, 0x10, 0x9A, 0xAC}

typedef enum {
    DFU_STATE_IDLE,    // 空闲状态，等待开始
    DFU_STATE_PREPARE, // 准备阶段
    DFU_STATE_HEADER,  // 接收块头
    DFU_STATE_DATA,    // 接收块数据
    DFU_STATE_VERIFY,  // 验签
    DFU_STATE_WRITE,   // 写入Flash
    DFU_STATE_FINAL,   // DFU结束
    DFU_STATE_ERROR    // 错误状态
} dfu_state;

typedef struct {
    uint8_t  signature[64]; // 当前块的 Curve25519 签名（示例为64字节）
    uint32_t block_size;    // 当前块实际大小（字节数，≤分块最大值）
} firmware_block_header;

typedef struct {
    dfu_state state;
    uint32_t  current_block_index;                       // 当前块
    uint32_t  total_block;                               // 总块数
    uint32_t  data_received;                             // 当前块已接收字节数
    uint32_t  flash_base_addr;                           // 固件写入的起始地址（如0x08008000）
    uint32_t  flash_offset;                              // 当前写入偏移
    bool      is_verified;                               // 当前块验签结果
    uint8_t*  public_key;                                // ECDSA公钥(Curve25519)
    uint8_t   header_buf[sizeof(firmware_block_header)]; // 块头缓存
    uint8_t   data_buf[DFU_PAGE_LEN];                    // 块数据缓存
} firmware_updater;

static volatile uint8_t ch;
static uint8_t          public_key[32] = ED25519_PUBKEY;

static firmware_updater dfu_updater;
static uint8_t          dfu_reset_task_index;
static uint8_t          dfu_process_task_index;

static uint8_t byte_count      = 0;
static uint8_t _total_block[4] = {0};

void DMA_Channel6_IRQHandler(void)
{
    if (DMA_GetFlagStatus(DMA_FLAG_TC6, DMA) != RESET) {
        switch (dfu_updater.state) {
            case DFU_STATE_IDLE: {
                static const uint8_t preamble[]   = DFU_PREAMBLE;
                static uint8_t       preamble_idx = 0;
                if (ch == preamble[preamble_idx++]) {
                    if (preamble_idx == sizeof(preamble)) {
                        dfu_updater.state               = DFU_STATE_PREPARE;
                        dfu_updater.data_received       = 0;
                        dfu_updater.current_block_index = 0;
                        preamble_idx                    = 0; // Reset for next DFU session
                    }
                } else {
                    preamble_idx = 0; // Reset if mismatch
                }
                break;
            }
            case DFU_STATE_PREPARE:
                _total_block[byte_count++] = ch;
                if (byte_count >= 4) {
                    dfu_updater.total_block = *(uint32_t*)_total_block;
                    dfu_updater.state       = DFU_STATE_HEADER;
                    byte_count              = 0;
                }
                break;
            case DFU_STATE_HEADER:
                if (dfu_updater.data_received < sizeof(firmware_block_header)) {
                    dfu_updater.header_buf[dfu_updater.data_received++] = ch;
                }
                break;
            case DFU_STATE_DATA:
                firmware_block_header* header = (firmware_block_header*)dfu_updater.header_buf;
                if (dfu_updater.data_received < header->block_size) {
                    dfu_updater.data_buf[dfu_updater.data_received++] = ch;
                }
                break;
            default:
                break;
        }

        DMA_ClrIntPendingBit(DMA_INT_TXC6, DMA);
        DMA_ClearFlag(DMA_FLAG_TC6, DMA);
    }
}

static void bootloader_response(dfu_state state)
{
    USART_SendData(UART_DFU, (uint16_t)state);
    while (USART_GetFlagStatus(UART_DFU, USART_FLAG_TXC) == RESET);
}

static void bootloader_dfu_reset(void)
{
    BOOT_LOG_WARN("long timer no byte,reset\r\n");
    flash_stop();
    NVIC_SystemReset();
}

static void bootloader_dfu_serial_init(void)
{
    uart_init(BLE_SERIAL);
    USART_Enable(UART_DFU, DISABLE);

    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA, ENABLE);
    DMA_InitType DMA_InitStructure;
    DMA_DeInit(DMA_CH6);
    DMA_InitStructure.PeriphAddr     = ((uint32_t)UART_DFU + 0x04);
    DMA_InitStructure.MemAddr        = (uint32_t)&ch;
    DMA_InitStructure.Direction      = DMA_DIR_PERIPH_SRC;
    DMA_InitStructure.BufSize        = 1;
    DMA_InitStructure.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    DMA_InitStructure.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;
    DMA_InitStructure.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    DMA_InitStructure.MemDataSize    = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.CircularMode   = DMA_MODE_CIRCULAR;
    DMA_InitStructure.Priority       = DMA_PRIORITY_VERY_HIGH;
    DMA_InitStructure.Mem2Mem        = DMA_M2M_DISABLE;
    DMA_Init(DMA_CH6, &DMA_InitStructure);
    DMA_ConfigInt(DMA_CH6, DMA_INT_TXC, ENABLE);
    DMA_RequestRemap(DMA_REMAP_USART2_RX, DMA, DMA_CH6, ENABLE);
    USART_EnableDMA(UART_DFU, USART_DMAREQ_RX, ENABLE);
    DMA_EnableChannel(DMA_CH6, ENABLE);
    USART_Enable(UART_DFU, ENABLE);

    NVIC_SetPriority(DMA_Channel6_IRQn, 0U);
    NVIC_EnableIRQ(DMA_Channel6_IRQn);
}

void bootloader_dfu_process(void)
{
    static dfu_state       last_state = DFU_STATE_IDLE;
    firmware_block_header* header     = (firmware_block_header*)dfu_updater.header_buf;
    switch (dfu_updater.state) {
        case DFU_STATE_PREPARE:
            dfu_reset_task_index = bootloader_systimer_add_task(bootloader_dfu_reset, 30000, false);
            break;
        case DFU_STATE_HEADER:
            if (dfu_updater.data_received >= sizeof(firmware_block_header)) {
                dfu_updater.data_received = 0;
                dfu_updater.state         = DFU_STATE_DATA;
            }
            break;
        case DFU_STATE_DATA:
            if (dfu_updater.data_received >= header->block_size) {
                dfu_updater.data_received = 0;
                dfu_updater.state         = DFU_STATE_VERIFY;
            }
            break;
        case DFU_STATE_VERIFY:
            // TODO:do verify
            dfu_updater.is_verified = true;
            if (!dfu_updater.is_verified) {
                dfu_updater.state = DFU_STATE_ERROR;
            } else {
                dfu_updater.state = DFU_STATE_WRITE;
            }
            break;
        case DFU_STATE_WRITE:
            uint32_t write_len = header->block_size < DFU_PAGE_LEN ? header->block_size : DFU_PAGE_LEN;
            flash_erase_page(dfu_updater.flash_base_addr + dfu_updater.flash_offset);
            for (uint32_t i = 0; i < write_len; i = i + 4) {
                uint32_t word_data = *(uint32_t*)&dfu_updater.data_buf[i];
                flash_program_word(dfu_updater.flash_base_addr + dfu_updater.flash_offset + i,
                                   word_data);
            }
            dfu_updater.flash_offset += DFU_PAGE_LEN;
            dfu_updater.state = DFU_STATE_HEADER; // next header
            memset(header, 0, sizeof(firmware_block_header));
            if (++dfu_updater.current_block_index == dfu_updater.total_block) {
                dfu_updater.state = DFU_STATE_FINAL;
            }
            break;
        case DFU_STATE_FINAL:
            flash_stop();
            NVIC_SystemReset();
            break;
        default:
            break;
    }

    if (last_state != dfu_updater.state) {
        bootloader_response(dfu_updater.state);
        bootloader_systimer_reset_task(dfu_reset_task_index);
        BOOT_LOG_VERBOSE("DFU state %d --> %d", last_state, dfu_updater.state);
        last_state = dfu_updater.state;
    }
}

void bootloader_dfu_init(void)
{
    if (*(uint32_t*)UPDATE_FLAG_ADDR == UPDATE_FLAG_MASK) {
        flash_start();
        flash_erase_page(UPDATE_FLAG_ADDR);
    }
    dfu_updater.state           = DFU_STATE_IDLE;
    dfu_updater.flash_base_addr = APP_START_ADDR;
    dfu_updater.public_key      = public_key;
    bootloader_dfu_serial_init();
    dfu_process_task_index = bootloader_systimer_add_task(bootloader_dfu_process, 5, true);
}
