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
 * File: $Id: porttimer_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions$
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mb_m.h"
#include "mbport.h"

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
/* ----------------------- Variables ----------------------------------------*/
static USHORT      usT35TimeOut50us;
static osTimerId_t timer;
static void        prvvTIMERExpiredISR(void);
static void        timer_timeout_ind(void* argument);

/* ----------------------- static functions ---------------------------------*/
static void prvvTIMERExpiredISR(void);

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortTimersInit(USHORT usTimeOut50us)
{
    /* backup T35 ticks */
    usT35TimeOut50us = usTimeOut50us;

    timer = osTimerNew(timer_timeout_ind, osTimerOnce, NULL, NULL);
    if (timer != NULL)
        return TRUE;
    else
        return FALSE;
}

void vMBMasterPortTimersT35Enable()
{
    uint32_t timer_tick = (50 * usT35TimeOut50us) / (1000 * 1000 / osKernelGetTickFreq()) + 3;

    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_T35);

    osTimerStart(timer, timer_tick);
}

void vMBMasterPortTimersConvertDelayEnable()
{
    uint32_t timer_tick = MB_MASTER_DELAY_MS_CONVERT * osKernelGetTickFreq() / 1000 + 3;

    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_CONVERT_DELAY);

    osTimerStart(timer, timer_tick);
}

void vMBMasterPortTimersRespondTimeoutEnable()
{
    uint32_t timer_tick = MB_MASTER_TIMEOUT_MS_RESPOND * osKernelGetTickFreq() / 1000 + 3;

    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_RESPOND_TIMEOUT);

    osTimerStart(timer, timer_tick);
}

void vMBMasterPortTimersDisable()
{
    osTimerStop(timer);
}

void prvvTIMERExpiredISR(void)
{
    pxMBMasterPortCBTimerExpired();
}

static void timer_timeout_ind(void* argument)
{
    prvvTIMERExpiredISR();
}

#endif
