#ifndef __LPUART_H__
#define __LPUART_H__

#include "board.h"

typedef struct lpuart_struct {
    GPIO_Module* tx_port;
    GPIO_Module* rx_port;
    uint32_t tx_pin;
    uint32_t rx_pin;
    uint32_t tx_gpio_clk;
    uint32_t rx_gpio_clk;
    uint8_t tx_af;
    uint8_t rx_af;
    uint8_t clk_src;
} lpuart_t;

void uart_init(void);

#endif /* __LPUART_H__ */