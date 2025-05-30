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
 * File: $Id: portserial_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions $
 */

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "board.h"
#include "ring_buffer.h"
#include "app_log.h"

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0

#define UART_DATA_LEN       256

#define MASTER_UART_TX_NEXT (1 << 0)
#define MASTER_UART_RX_COME (1 << 1)
#define MASTER_UART_ERR     (1 << 2)

static uint8_t          buf;
static osEventFlagsId_t masterUartEvent;
uart_regs_t*            masterDev;
uint8_t                 masterTxRingBuf[UART_DATA_LEN] = {0};
uint8_t                 masterRxRingBuf[UART_DATA_LEN] = {0};
static ring_buffer_t    masterRxRingBufCB;

app_uart_params_t masterUartParams = {
    .id      = APP_UART_ID_1,
    .pin_cfg = {
        .tx = {
            .type = APP_IO_TYPE_NORMAL,
            .mux  = APP_IO_MUX_1,
            .pin  = APP_IO_PIN_30,
            .pull = APP_IO_PULLUP,
        },
        .rx = {
            .type = APP_IO_TYPE_NORMAL,
            .mux  = APP_IO_MUX_1,
            .pin  = APP_IO_PIN_26,
            .pull = APP_IO_PULLUP,
        },
    },
    .init = {
        .baud_rate       = 9600,
        .data_bits       = UART_DATABITS_8,
        .stop_bits       = UART_STOPBITS_1,
        .parity          = UART_PARITY_NONE,
        .hw_flow_ctrl    = UART_HWCONTROL_NONE,
        .rx_timeout_mode = UART_RECEIVER_TIMEOUT_ENABLE,
    },
};

void appMasterUartCallback(app_uart_evt_t* p_evt)
{
    if (masterUartEvent != NULL) {
        if (p_evt->type == APP_UART_EVT_TX_CPLT) {
            osEventFlagsSet(masterUartEvent, MASTER_UART_TX_NEXT);
        }
        if (p_evt->type == APP_UART_EVT_RX_DATA) {
            ring_buffer_write(&masterRxRingBufCB, &buf, 1);
            app_uart_receive_async(masterUartParams.id, &buf, 1);
            osEventFlagsSet(masterUartEvent, MASTER_UART_RX_COME);
        }
        if (p_evt->type == APP_UART_EVT_ERROR) {
            osEventFlagsSet(masterUartEvent, MASTER_UART_ERR);
        }

        return;
    }
}

void masterUartTask(void* p)
{
    uint32_t evRecv;
    while (1) {
        evRecv = xEventGroupWaitBits(masterUartEvent,
                                     MASTER_UART_TX_NEXT | MASTER_UART_RX_COME | MASTER_UART_ERR,
                                     osFlagsWaitAny,
                                     osWaitForever);
        if (evRecv & MASTER_UART_TX_NEXT) {
            pxMBMasterFrameCBTransmitterEmpty();
        }
        if (evRecv & MASTER_UART_RX_COME) {
            pxMBMasterFrameCBByteReceived();
        }
    }
}

BOOL xMBMasterPortSerialInit(UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity)
{
    uint16_t          ret         = 0;
    app_uart_tx_buf_t uart_buffer = {0};
    uart_buffer.tx_buf            = masterTxRingBuf;
    uart_buffer.tx_buf_size       = sizeof(masterTxRingBuf);

    /** please configure uart config at the head **/
    ret = app_uart_init(&masterUartParams, appMasterUartCallback, &uart_buffer);
    if (ret != APP_DRV_SUCCESS) {
        return FALSE;
    }

    masterDev = app_uart_get_handle(masterUartParams.id)->p_instance;
    ring_buffer_init(&masterRxRingBufCB, masterRxRingBuf, sizeof(masterRxRingBuf));
    masterUartEvent = osEventFlagsNew(NULL);

    osThreadAttr_t masterUartTaskParams = {
        .name       = "slaveUartTask",
        .stack_size = 512,
        .priority   = osPriorityRealtime,
    };
    osThreadNew((osThreadFunc_t)masterUartTask, NULL, &masterUartTaskParams);
    return TRUE;
}

void vMBMasterPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable)
{
    if (xRxEnable) {
        app_uart_receive_async(masterUartParams.id, &buf, 1);
    } else {
        app_uart_abort_receive(masterUartParams.id);
    }
    if (xTxEnable) {
        osEventFlagsSet(masterUartEvent, MASTER_UART_TX_NEXT);
    } else {
        osEventFlagsClear(masterUartEvent, MASTER_UART_TX_NEXT);
    }
    return;
}

void vMBMasterPortClose(void)
{
    app_uart_deinit(masterUartParams.id);
}

BOOL xMBMasterPortSerialPutByte(CHAR ucByte)
{
    app_uart_transmit_async(masterUartParams.id, &ucByte, 1);
    return TRUE;
}

BOOL xMBMasterPortSerialGetByte(CHAR* pucByte)
{
    ring_buffer_read(&masterRxRingBufCB, pucByte, 1);
    return TRUE;
}

#endif
