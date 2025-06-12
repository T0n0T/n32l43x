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

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- static functions ---------------------------------*/
static void prvvTIMERExpiredISR(void);
/* ----------------------- Start implementation -----------------------------*/
BOOL xMBPortTimersInit(USHORT usTim1Timerout50us)
{
    /* Initializes the module. */
    NVIC_InitType NVIC_InitStructure;

    /* Enable the TIM1 global Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel            = TIM1_UP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd         = ENABLE;

    NVIC_Init(&NVIC_InitStructure);
    TIM_TimeBaseInitType TIM_TimeBaseStructure;

    /* TIM1 clock enable */
    RCC_ConfigTim18Clk(RCC_TIM18CLK_SRC_TIM18CLK);
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_TIM1, ENABLE);

    /* Time base configuration 
    @tip when using higher clk like pll, 
    oneshot update willbe overrun,
    so the expired also need to edit*/
    TIM_TimeBaseStructure.Period    = 50 * 16 * usTim1Timerout50us - 1;
    TIM_TimeBaseStructure.Prescaler = 0;
    TIM_TimeBaseStructure.ClkDiv    = 0;
    TIM_TimeBaseStructure.CntMode   = TIM_CNT_MODE_UP;

    TIM_InitTimeBase(TIM1, &TIM_TimeBaseStructure);

    /* Prescaler configuration */
    TIM_ConfigPrescaler(TIM1, 0, TIM_PSC_RELOAD_MODE_IMMEDIATE);
    /* TIM1 enable update irq */
    TIM_ConfigInt(TIM1, TIM_INT_UPDATE, ENABLE);
    return TRUE;
}

void vMBPortTimersEnable()
{
    /* Enable the timer with the timeout passed to xMBPortTimersInit( ) */
    /* Read the current counter value. Counter value is in status.counter. */

    TIM_SetCnt(TIM1, 0);
    /* TIM1 enable counter */
    TIM_Enable(TIM1, ENABLE);
}

void vMBPortTimersDisable()
{
    /* Read the current counter value. Counter value is in status.counter. */
    /* TIM1 disable counter */
    TIM_Enable(TIM1, DISABLE);
}

/* Create an ISR which is called whenever the timer has expired. This function
 * must then call pxMBPortCBTimerExpired( ) to notify the protocol stack that
 * the timer has expired.
 */
static void prvvTIMERExpiredISR(void)
{
    (void)pxMBPortCBTimerExpired();
}

/**
 * @brief  This function handles TIM1 update interrupt request.
 */
void TIM1_UP_IRQHandler(void)
{
    if (TIM_GetIntStatus(TIM1, TIM_INT_UPDATE) != RESET) {
        TIM_ClrIntPendingBit(TIM1, TIM_INT_UPDATE);
        prvvTIMERExpiredISR();
    }
}