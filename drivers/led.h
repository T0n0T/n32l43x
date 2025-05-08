#ifndef __LED_H__
#define __LED_H__

#include "board.h"

#define RUN_LED_PORT GPIOB
#define RUN_LED_PIN  GPIO_PIN_4

typedef enum led_index {
    LED_1 = 0,
    LED_2,
    LED_3,
    LED_MAX,
} led_index_t;

typedef struct led_struct {
    GPIO_Module* port;
    uint32_t clk;
    uint16_t pin;
    uint16_t active_level;
} led_t;

void led_init(void);
void led_on(led_index_t index);
void led_off(led_index_t index);
void led_toggle(led_index_t index);

#endif /* __LED_H__ */

