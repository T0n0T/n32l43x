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
 * File: $Id: portserial.c,v 1.60 2013/08/13 15:07:05 Armink $
 */

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "board.h"
#include "ring_buffer.h"
#include "app_log.h"

#define UART_DATA_LEN      256

#define SLAVE_UART_TX_NEXT (1 << 0)
#define SLAVE_UART_RX_COME (1 << 1)
#define SLAVE_UART_ERR     (1 << 2)

static uint8_t          buf;
static osEventFlagsId_t slaveUartEvent;
uart_regs_t*            slaveDev;
uint8_t                 slaveTxRingBuf[UART_DATA_LEN] = {0};
uint8_t                 slaveRxRingBuf[UART_DATA_LEN] = {0};
static ring_buffer_t    slaveRxRingBufCB;

app_uart_params_t slaveUartParams = {
    .id      = APP_UART_ID_1,
    .pin_cfg = {
        .tx = {
            .type = APP_UART1_TX_IO_TYPE,
            .mux  = APP_UART1_TX_PINMUX,
            .pin  = APP_UART1_TX_PIN,
            .pull = APP_IO_PULLUP,
        },
        .rx = {
            .type = APP_UART1_RX_IO_TYPE,
            .mux  = APP_UART1_RX_PINMUX,
            .pin  = APP_UART1_RX_PIN,
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

void appSlaveUartCallback(app_uart_evt_t* p_evt)
{
    if (slaveUartEvent != NULL) {
        if (p_evt->type == APP_UART_EVT_TX_CPLT) {
            osEventFlagsSet(slaveUartEvent, SLAVE_UART_TX_NEXT);
        }
        if (p_evt->type == APP_UART_EVT_RX_DATA) {
            ring_buffer_write(&slaveRxRingBufCB, &buf, 1);
            app_uart_receive_async(slaveUartParams.id, &buf, 1);
            osEventFlagsSet(slaveUartEvent, SLAVE_UART_RX_COME);
        }
        if (p_evt->type == APP_UART_EVT_ERROR) {
            osEventFlagsSet(slaveUartEvent, SLAVE_UART_ERR);
        }
        return;
    }
}

void slaveUartTask(void* p)
{
    uint32_t evRecv;
    while (1) {
        evRecv = osEventFlagsWait(slaveUartEvent,
                                  SLAVE_UART_TX_NEXT | SLAVE_UART_RX_COME | SLAVE_UART_ERR,
                                  osFlagsWaitAny | osFlagsNoClear,
                                  osWaitForever);
        if (evRecv & SLAVE_UART_TX_NEXT) {
            pxMBFrameCBTransmitterEmpty();
        }
        if (evRecv & SLAVE_UART_RX_COME) {
            pxMBFrameCBByteReceived();
            osEventFlagsClear(slaveUartEvent, SLAVE_UART_RX_COME);
        }
        if (evRecv & SLAVE_UART_ERR) {
            osEventFlagsClear(slaveUartEvent, SLAVE_UART_ERR);
        }
    }
}

BOOL xMBPortSerialInit(UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity)
{
    uint16_t          ret         = 0;
    app_uart_tx_buf_t uart_buffer = {0};
    uart_buffer.tx_buf            = slaveTxRingBuf;
    uart_buffer.tx_buf_size       = sizeof(slaveTxRingBuf);

    app_io_init_t io_init;

    // RS485 使能引脚
    io_init.pin  = APP_RS485_EN;
    io_init.mode = APP_IO_MODE_OUTPUT;
    io_init.pull = APP_IO_PULLUP;
    io_init.mux  = APP_IO_MUX;
    app_io_init(APP_RS485_TYPE, &io_init);
    app_io_set_speed(APP_RS485_TYPE, APP_RS485_EN, APP_IO_SPPED_HIGH);
    // 低电平为接收状态
    app_io_write_pin(APP_RS485_TYPE, APP_RS485_EN, APP_IO_PIN_RESET);

    /** please configure uart config at the head **/
    ret = app_uart_init(&slaveUartParams, appSlaveUartCallback, &uart_buffer);
    if (ret != APP_DRV_SUCCESS) {
        return FALSE;
    }

    slaveDev = app_uart_get_handle(slaveUartParams.id)->p_instance;
    ring_buffer_init(&slaveRxRingBufCB, slaveRxRingBuf, sizeof(slaveRxRingBuf));
    slaveUartEvent = osEventFlagsNew(NULL);

    osThreadAttr_t slaveUartTaskParams = {
        .name       = "slaveUartTask",
        .stack_size = 512,
        .priority   = osPriorityRealtime,
    };
    osThreadNew((osThreadFunc_t)slaveUartTask, NULL, &slaveUartTaskParams);
    return TRUE;
}

void vMBPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable)
{
    if (xRxEnable) {
        app_uart_receive_async(slaveUartParams.id, &buf, 1);
    } else {
        app_uart_abort_receive(slaveUartParams.id);
    }
    if (xTxEnable) {
        app_io_write_pin(APP_RS485_TYPE, APP_RS485_EN, APP_IO_PIN_SET);
        osEventFlagsSet(slaveUartEvent, SLAVE_UART_TX_NEXT);
    } else {
        app_io_write_pin(APP_RS485_TYPE, APP_RS485_EN, APP_IO_PIN_RESET);
        osEventFlagsClear(slaveUartEvent, SLAVE_UART_TX_NEXT);
    }
    return;
}

void vMBPortClose(void)
{
    app_uart_deinit(slaveUartParams.id);
}

BOOL xMBPortSerialPutByte(CHAR ucByte)
{
    app_uart_transmit_sync(slaveUartParams.id, &ucByte, 1, 100);
    // APP_LOG_RAW_INFO("%02x ", ucByte);
    return TRUE;
}

BOOL xMBPortSerialGetByte(CHAR* pucByte)
{
    ring_buffer_read(&slaveRxRingBufCB, pucByte, 1);
    // APP_LOG_RAW_INFO("%02x ", *pucByte);
    return TRUE;
}