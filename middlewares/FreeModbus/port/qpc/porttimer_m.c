/*
 * FreeModbus Libary: QPC Port
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 * Modified for QPC QV kernel
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
#include "SEGGER_SYSVIEW.h"
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mb_m.h"
#include "mbport.h"

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0

/* ----------------------- static functions ---------------------------------*/
USHORT               p_usTimeOut50us;
TIM_TimeBaseInitType TIM_TimeBaseStructure;

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortTimersInit(USHORT usTimeOut50us)
{
    p_usTimeOut50us = usTimeOut50us;

    /* TIM1 clock enable */
    RCC_ConfigTim18Clk(RCC_TIM18CLK_SRC_TIM18CLK);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_TIM1, ENABLE);

    TIM_ConfigInt(TIM1, TIM_INT_UPDATE, ENABLE);
    NVIC_EnableIRQ(TIM1_UP_IRQn);
    return TRUE;
}

void vMBMasterPortTimersDisable()
{
    /* Read the current counter value. Counter value is in status.counter. */
    /* TIM1 disable counter */
    TIM_Enable(TIM1, DISABLE);
}

void vMBMasterPortTimersT35Enable()
{
    vMBMasterSetCurTimerMode(MB_TMODE_T35);
    TIM_TimeBaseStructure.Period    = 50 * p_usTimeOut50us - 1;
    TIM_TimeBaseStructure.Prescaler = SystemCoreClock / 1000000 - 1;
    TIM_TimeBaseStructure.ClkDiv    = 0;
    TIM_TimeBaseStructure.CntMode   = TIM_CNT_MODE_UP;
    TIM_InitTimeBase(TIM1, &TIM_TimeBaseStructure);
    TIM_SetCnt(TIM1, 0);
    TIM_Enable(TIM1, ENABLE);
}

void vMBMasterPortTimersConvertDelayEnable()
{
    TIM_Enable(TIM1, DISABLE);
    vMBMasterSetCurTimerMode(MB_TMODE_CONVERT_DELAY);
    TIM_TimeBaseStructure.Period    = MB_MASTER_DELAY_MS_CONVERT - 1;
    TIM_TimeBaseStructure.Prescaler = SystemCoreClock / 1000 - 1;
    TIM_TimeBaseStructure.ClkDiv    = 0;
    TIM_TimeBaseStructure.CntMode   = TIM_CNT_MODE_UP;
    TIM_InitTimeBase(TIM1, &TIM_TimeBaseStructure);
    TIM_SetCnt(TIM1, 0);
    TIM_Enable(TIM1, ENABLE);
}

void vMBMasterPortTimersRespondTimeoutEnable()
{
    TIM_Enable(TIM1, DISABLE);
    vMBMasterSetCurTimerMode(MB_TMODE_RESPOND_TIMEOUT);
    TIM_TimeBaseStructure.Period    = MB_MASTER_TIMEOUT_MS_RESPOND - 1;
    TIM_TimeBaseStructure.Prescaler = SystemCoreClock / 1000 - 1;
    TIM_TimeBaseStructure.ClkDiv    = 0;
    TIM_TimeBaseStructure.CntMode   = TIM_CNT_MODE_UP;
    TIM_InitTimeBase(TIM1, &TIM_TimeBaseStructure);
    TIM_SetCnt(TIM1, 0);
    TIM_Enable(TIM1, ENABLE);
}

/**
 * @brief  This function handles TIM1 update interrupt request.
 */
void TIM1_UP_IRQHandler(void)
{
    if (TIM_GetIntStatus(TIM1, TIM_INT_UPDATE) != RESET) {
        TIM_ClrIntPendingBit(TIM1, TIM_INT_UPDATE);
        pxMBMasterPortCBTimerExpired();
    }
}

#endif