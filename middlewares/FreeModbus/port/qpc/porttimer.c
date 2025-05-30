/*
 * FreeModbus Libary: RT-Thread Port
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: porttimer.c,v 1.60 2013/08/13 15:07:05 Armink $
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- static functions ---------------------------------*/
static osTimerId_t timer;
static uint32_t    timer_tick;
static void        prvvTIMERExpiredISR(void);
static void        timer_timeout_ind(void* argument);
/* ----------------------- Start implementation -----------------------------*/
BOOL xMBPortTimersInit(USHORT usTimeOut50us)
{
    /** for this board, using 2 ticks for modifying*/
    timer      = osTimerNew(timer_timeout_ind, osTimerOnce, NULL, NULL);
    timer_tick = (50 * usTimeOut50us) / (1000 * 1000 / osKernelGetTickFreq()) + 3;
    if (timer != NULL)
        return TRUE;
    else
        return FALSE;
}

void vMBPortTimersEnable()
{
    osTimerStart(timer, timer_tick);
}

void vMBPortTimersDisable()
{
    osTimerStop(timer);
}

void prvvTIMERExpiredISR(void)
{
    (void)pxMBPortCBTimerExpired();
}

static void timer_timeout_ind(void* argument)
{
    prvvTIMERExpiredISR();
}
