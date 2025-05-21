#ifndef __LPTIMER_H__
#define __LPTIMER_H__

#include "board.h"

#define LPTIMER_FREQ 40000 // LPTIMER时钟频率（LSI）
#define LPTIMER_MS_TO_TICKS(ms) ((LPTIMER_FREQ * (ms)) / 1000) // 毫秒转LPTIMER计数值

typedef void (*lptimer_irq_callback_t)(void);

void lptimer_init(void);
void lptimer_start(uint32_t cnt, lptimer_irq_callback_t cb);
void lptimer_stop(void);

#endif /* __LPTIMER_H__ */