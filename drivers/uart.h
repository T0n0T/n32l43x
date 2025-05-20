#ifndef __UART_H__
#define __UART_H__

#include "board.h"

typedef enum uart_index {
    CONSOLE = 0,
    RESERVED,
    BUS_485,
    UART_MAX,
} uart_index_t;

typedef struct uart_struct {
    GPIO_Module*  tx_port;
    GPIO_Module*  rx_port;
    uint32_t      tx_pin;
    uint32_t      rx_pin;
    uint32_t      tx_gpio_clk;
    uint32_t      rx_gpio_clk;
    uint8_t       tx_af;
    uint8_t       rx_af;
    USART_Module* handle;
    IRQn_Type     irqn;
    uint32_t      baudrate;
    uint32_t      clk_src;
    uint8_t       parity;
    uint8_t       stop_bits;
} uart_t;

void uart_init(void);

#endif /* __UART_H__ */