#ifndef __AT_H__
#define __AT_H__

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

#define AT_PWR_PORT         GPIOB
#define AT_PWR_CLK          RCC_APB2_PERIPH_GPIOB
#define AT_PWR_PIN          GPIO_PIN_2
#define AT_PWR_HIGH         AT_PWR_PORT->PBSC = AT_PWR_PIN;
#define AT_PWR_LOW          AT_PWR_PORT->PBC = AT_PWR_PIN;

#define AT                  LORAWAN
#define USART_AT            USART3
#define USART_AT_IRQn       USART3_IRQn
#define USART_AT_IRQHandler USART3_IRQHandler
#define USART_AT_DMA_TX     DMA_CH7
#define USART_AT_DMA_TX_MAP DMA_REMAP_USART3_TX
#define USART_AT_DMA_RX     DMA_CH8
#define USART_AT_DMA_RX_MAP DMA_REMAP_USART3_RX
#define AT_BUF_LEN          64U

#endif /* __AT_H__ */