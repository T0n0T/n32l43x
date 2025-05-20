#ifndef __HALL_H__
#define __HALL_H__

#include "board.h"

typedef enum hall_index {
    HALL_1 = 0,
    HALL_2,
    HALL_3,
    HALL_4,
    HALL_5,
    HALL_6,
    HALL_MAX,
} hall_index_t;

typedef struct hall_struct {
    GPIO_Module* port;
    uint32_t     exit_Line;
    uint32_t     clk;
    uint16_t     pin;
    uint8_t      exit_irq;
    uint8_t      active_level;
} hall_t;

void hall_init(void);
void hall_set_ctr(uint8_t state);
uint8_t hall_read(hall_index_t index);

#endif /* __HALL_H__ */