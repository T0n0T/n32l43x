#ifndef UART2_DMA_H
#define UART2_DMA_H

#include <stdbool.h>
#include <stdint.h>

#define UART2_DMA_RX_SIZE 2048U

/* Called from the IDLE interrupt, including empty/error frames. Data is only
 * valid during the callback; do not block or reconfigure the UART here. */
typedef void (*uart2_dma_rx_handler_t)(const uint8_t* data, uint16_t len, bool rx_error);

void uart2_dma_init(uint32_t baudrate, uart2_dma_rx_handler_t handler);
void uart2_dma_deinit(void);
/* Foreground-only operations. Changing baud/handler discards pending RX data. */
bool     uart2_dma_set_baudrate(uint32_t baudrate);
uint32_t uart2_dma_get_baudrate(void);
void     uart2_dma_set_rx_handler(uart2_dma_rx_handler_t handler);
void     uart2_dma_write(const uint8_t* data, uint16_t len);

#endif
