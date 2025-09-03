/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*            (c) 1995 - 2024 SEGGER Microcontroller GmbH             *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       SEGGER SystemView * Real-time application analysis           *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* SEGGER strongly recommends to not make any changes                 *
* to or modify the source code of this software in order to stay     *
* compatible with the SystemView and RTT protocol, and J-Link.       *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* o Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
*                                                                    *
*       SystemView version: 3.60e                                    *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : SEGGER_SYSVIEW_Config_NoOS.c
Purpose : Sample setup configuration of SystemView without an OS.
Revision: $Rev: 9599 $
*/
#include "SEGGER_SYSVIEW.h"
#include "SEGGER_SYSVIEW_Conf.h"

// SystemcoreClock can be used in most CMSIS compatible projects.
// In non-CMSIS projects define SYSVIEW_CPU_FREQ.
extern unsigned int SystemCoreClock;

/*********************************************************************
 *
 *       Defines, configurable
 *
 **********************************************************************
 */
// The application name to be displayed in SystemViewer
#define SYSVIEW_APP_NAME       "VALVE Application"

// The target device name
#define SYSVIEW_DEVICE_NAME    "N32L436RB"

// Frequency of the timestamp. Must match SEGGER_SYSVIEW_Conf.h
#define SYSVIEW_TIMESTAMP_FREQ (SystemCoreClock)

// System Frequency. SystemcoreClock is used in most CMSIS compatible projects.
#define SYSVIEW_CPU_FREQ       (SystemCoreClock)

// The lowest RAM address used for IDs (pointers)
#define SYSVIEW_RAM_BASE       (0x20000000)

// Define as 1 if the Cortex-M cycle counter is used as SystemView timestamp. Must match SEGGER_SYSVIEW_Conf.h
#ifndef USE_CYCCNT_TIMESTAMP
#define USE_CYCCNT_TIMESTAMP 1
#endif

// Define as 1 if the Cortex-M cycle counter is used and there might be no debugger attached while recording.
#ifndef ENABLE_DWT_CYCCNT
#define ENABLE_DWT_CYCCNT (USE_CYCCNT_TIMESTAMP & SEGGER_SYSVIEW_POST_MORTEM_MODE)
#endif

/*********************************************************************
 *
 *       Defines, fixed
 *
 **********************************************************************
 */
#define DEMCR         (*(volatile unsigned long*)(0xE000EDFCuL)) // Debug Exception and Monitor Control Register
#define TRACEENA_BIT  (1uL << 24)                                // Trace enable bit
#define DWT_CTRL      (*(volatile unsigned long*)(0xE0001000uL)) // DWT Control Register
#define NOCYCCNT_BIT  (1uL << 25)                                // Cycle counter support bit
#define CYCCNTENA_BIT (1uL << 0)                                 // Cycle counter enable bit

SEGGER_SYSVIEW_TASKINFO _Q_taskInfo[5];

static const SEGGER_SYSVIEW_DATA_REGISTER hallData = {
    .ID            = 0,
    .DataType      = SEGGER_SYSVIEW_TYPE_U32,
    .Offset        = 0,
    .RangeMin      = 0,
    .RangeMax      = 0,
    .ScalingFactor = 1.0f, /* important: set to non-zero! */
    .sName         = "hall status",
    .sUnit         = ""};
static const SEGGER_SYSVIEW_DATA_REGISTER valTick = {
    .ID            = 1,
    .DataType      = SEGGER_SYSVIEW_TYPE_I32,
    .Offset        = 0,
    .RangeMin      = 0,
    .RangeMax      = 0,
    .ScalingFactor = 1.0f, /* important: set to non-zero! */
    .sName         = "valve tick",
    .sUnit         = ""};
/*********************************************************************
 *
 *       _cbSendSystemDesc()
 *
 *  Function description
 *    Sends SystemView description strings.
 */
static void _cbSendSystemDesc(void)
{
    SEGGER_SYSVIEW_SendSysDesc("N=" SYSVIEW_APP_NAME ",D=" SYSVIEW_DEVICE_NAME);
    SEGGER_SYSVIEW_SendSysDesc("I#15=SysTick");
    SEGGER_SYSVIEW_NameMarker(0x400, "VALVE_IDLE");
    SEGGER_SYSVIEW_NameMarker(0x401, "VALVE_UPDATE");
    SEGGER_SYSVIEW_NameMarker(0x402, "VALVE_PERSIST_START");
    SEGGER_SYSVIEW_NameMarker(0x403, "VALVE_PERSIST_EARSE");
    SEGGER_SYSVIEW_NameMarker(0x404, "VALVE_PERSIST_PROGRAM");
    SEGGER_SYSVIEW_NameMarker(0x405, "VALVE_PERSIST_DONE");
    SEGGER_SYSVIEW_NameMarker(0x406, "VALVE_AT_SEND");
    SEGGER_SYSVIEW_NameMarker(0x407, "VALVE_AT_RESP");
    SEGGER_SYSVIEW_NameMarker(0x408, "VALVE_TRANSFER_PREP");
    SEGGER_SYSVIEW_NameMarker(0x409, "VALVE_TRANSFER_TRANS");
    SEGGER_SYSVIEW_RegisterData(&hallData);
    SEGGER_SYSVIEW_RegisterData(&valTick);
}

static void _cbSendTaskList(void)
{
    for (int i = 0; i < sizeof(_Q_taskInfo) / sizeof(_Q_taskInfo[0]); i++) {
        SEGGER_SYSVIEW_SendTaskInfo(&_Q_taskInfo[i]);
    }
}

static const SEGGER_SYSVIEW_OS_API _NoOSAPI = {0, _cbSendTaskList};
/*********************************************************************
 *
 *       Global functions
 *
 **********************************************************************
 */
void SEGGER_SYSVIEW_Conf(void)
{
#if USE_CYCCNT_TIMESTAMP
#if ENABLE_DWT_CYCCNT
    //
    // If no debugger is connected, the DWT must be enabled by the application
    //
    if ((DEMCR & TRACEENA_BIT) == 0) {
        DEMCR |= TRACEENA_BIT;
    }
#endif
    //
    //  The cycle counter must be activated in order
    //  to use time related functions.
    //
    if ((DWT_CTRL & NOCYCCNT_BIT) == 0) {      // Cycle counter supported?
        if ((DWT_CTRL & CYCCNTENA_BIT) == 0) { // Cycle counter not enabled?
            DWT_CTRL |= CYCCNTENA_BIT;         // Enable Cycle counter
        }
    }
#endif
    SEGGER_SYSVIEW_Init(SYSVIEW_TIMESTAMP_FREQ, SYSVIEW_CPU_FREQ,
                        &_NoOSAPI, _cbSendSystemDesc);
    SEGGER_SYSVIEW_SetRAMBase(SYSVIEW_RAM_BASE);
}

/*************************** End of file ****************************/
