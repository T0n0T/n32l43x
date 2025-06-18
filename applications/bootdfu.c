#include "bootloader.h"
#include "uart.h"
#include "flash.h"

#define UART_DFU   USART2
#define FW_BLK_LEN 2048

typedef enum {
    RECV_STATE_IDLE,   // 空闲状态，等待开始
    RECV_STATE_HEADER, // 接收块头（FirmwareBlockHeader）
    RECV_STATE_DATA,   // 接收块数据
    RECV_STATE_VERIFY, // 验签和哈希校验
    RECV_STATE_WRITE,  // 写入Flash
    RECV_STATE_ERROR   // 错误状态
} RecvState;

typedef struct {
    uint32_t block_id;      // 块序号（从0开始）
    uint32_t block_size;    // 当前块实际大小（字节数，≤分块最大值）
    uint8_t  hash[32];      // 当前块的SHA-256哈希值
    uint8_t  signature[64]; // 当前块的ECDSA签名（示例为64字节）
} firmware_block_header;

typedef struct {
    RecvState state;
    uint32_t  current_block_id;                             // 当前处理的块ID
    uint32_t  total_blocks;                                 // 总块数（由首块或协议确定）
    uint8_t   header_buf[sizeof(firmware_block_header)];    // 块头缓存
    uint8_t   data_buf[FW_BLK_LEN];                         // 块数据缓存
    uint32_t  data_received;                                // 当前块已接收字节数
    uint8_t   public_key[64];                               // ECDSA公钥（预置）
    bool      is_verified;                                  // 当前块验签结果
    uint32_t  flash_base_addr;                              // 固件写入的起始地址（如0x08008000）
    uint32_t  flash_offset;                                 // 当前写入偏移
    void (*on_block_done)(uint32_t block_id, bool success); // 块处理完成回调
} firmware_updater;

static uint8_t  _dfu_ch;
static uint32_t _dfu_pos;
static uint8_t  _dfu_buf[FW_BLK_LEN];

void DMA_Channel6_IRQHandler(void)
{
    if (DMA_GetFlagStatus(DMA_FLAG_TC6, DMA) != RESET) {
        if (_dfu_pos < FW_BLK_LEN - 1) {
            _dfu_buf[_dfu_pos] = _dfu_ch;
            _dfu_pos++;
        }
        DMA_ClrIntPendingBit(DMA_INT_TXC6, DMA);
        DMA_ClearFlag(DMA_FLAG_TC6, DMA);
    }
}

static void bootloader_dfu_serial_init(void)
{
    uart_init(BLE_SERIAL);
    USART_Enable(UART_DFU, DISABLE);

    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA, ENABLE);
    DMA_InitType DMA_InitStructure;
    DMA_DeInit(DMA_CH6);
    DMA_InitStructure.PeriphAddr     = ((uint32_t)UART_DFU + 0x04);
    DMA_InitStructure.MemAddr        = (uint32_t)&_dfu_ch;
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

void bootloader_dfu_init(void)
{
    bootloader_dfu_serial_init();
}
