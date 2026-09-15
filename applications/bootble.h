#ifndef BOOTBLE_H
#define BOOTBLE_H

#include <stdbool.h>
#include <stdint.h>

void bootloader_ble_init(uint32_t now_ms);
void bootloader_ble_power_on(void);
/* Power off before APP handoff so APP can receive its own startup banner. */
void bootloader_ble_deinit(void);
void bootloader_ble_process(uint32_t now_ms);
bool bootloader_ble_is_finished(void);
bool bootloader_ble_is_ready(void);

#endif
