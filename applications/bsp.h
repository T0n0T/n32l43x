//============================================================================
// "Blinky" example
//
// Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
//
//                    Q u a n t u m  L e a P s
//                    ------------------------
//                    Modern Embedded Software
//
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
//
// This software is dual-licensed under the terms of the open-source GNU
// General Public License (GPL) or under the terms of one of the closed-
// source Quantum Leaps commercial licenses.
//
// Redistributions in source code must retain this top-level comment block.
// Plagiarizing this software to sidestep the license obligations is illegal.
//
// NOTE:
// The GPL does NOT permit the incorporation of this code into proprietary
// programs. Please contact Quantum Leaps for commercial licensing options,
// which expressly supersede the GPL and are designed explicitly for
// closed-source distribution.
//
// Quantum Leaps contact information:
// <www.state-machine.com/licensing>
// <info@state-machine.com>
//============================================================================
#ifndef BSP_H_
#define BSP_H_

#include "board.h"
#include "flash.h"
#include "flash_noblock.h"
#include "hall.h"
#include "lcd.h"
#include "led.h"
#include "lptimer.h"
#include "rtc.h"
#include "spi_flash.h"
#include "uart.h"

#define DEF_ISR_PRI       5
#define TICK_RATE         1000
#define LPTIM_INTERVAL_MS 1000
#define LPTIM_REPORT_MS   60000
#define MS_TO_TICK(ms)    ((ms) * TICK_RATE / 1000)

extern volatile bool pvd_is_power_low;
extern volatile bool run_is_reporting;
extern volatile bool transfer_is_error;

extern int g_cmd_id;
extern int g_counter_id;
extern int g_handle_id;
extern int g_persist_id;

void BSP_init(void);
void BSP_init_ext(void);
void BSP_start(void);

#endif // BSP_H_