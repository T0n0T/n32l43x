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
 * File: $Id: portserial.c,v 1.60 2013/08/13 15:07:05 Armink $
 */

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

#if MB_SLAVE_RTU_ENABLED > 0 || MB_SLAVE_ASCII_ENABLED > 0

/* ----------------------- Static variables ---------------------------------*/
static volatile CHAR modbus_rx_char  = 0;

/* ----------------------- User defenitions ---------------------------------*/
#define RS485_RTS_PORT          GPIOB
#define RS485_RTS_CLK           RCC_APB2_PERIPH_GPIOB
#define RS485_RTS_PIN           GPIO_PIN_3
#define RS485_RTS_HIGH          RS485_RTS_PORT->PBSC = RS485_RTS_PIN;
#define RS485_RTS_LOW           RS485_RTS_PORT->PBC = RS485_RTS_PIN;

#define USART_MODBUS            UART5
#define USART_MODBUS_IRQn       UART5_IRQn
#define USART_MODBUS_IRQHandler UART5_IRQHandler

static void prvvUARTTxReadyISR(void);
static void prvvUARTRxISR(void);

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBPortSerialInit(UCHAR ucPort, ULONG ulBaudRate,
                       UCHAR ucDataBits, eMBParity eParity)
{
    // GPIO_InitType GPIO_InitStructure;
    // GPIO_InitStruct(&GPIO_InitStructure);
    // RCC_EnableAPB2PeriphClk(RS485_RTS_CLK, ENABLE);
    // GPIO_InitStructure.Pin       = RS485_RTS_PIN;
    // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    // GPIO_InitPeripheral(RS485_RTS_PORT, &GPIO_InitStructure);

    uart_init(BUS_485);

    NVIC_EnableIRQ(USART_MODBUS_IRQn);

    return TRUE;
}

void vMBPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable)
{
    if (xRxEnable) {
        USART_ClrIntPendingBit(USART_MODBUS, USART_INT_RXDNE);
        USART_ClrFlag(USART_MODBUS, USART_FLAG_RXDNE);
        USART_ConfigInt(USART_MODBUS, USART_INT_RXDNE, ENABLE);
        // RS485_RTS_LOW;
    } else {
        USART_ConfigInt(USART_MODBUS, USART_INT_RXDNE, DISABLE);
    }

    if (xTxEnable) {
        // RS485_RTS_HIGH;
        USART_ClrIntPendingBit(USART_MODBUS, USART_INT_TXDE);
        USART_ClrFlag(USART_MODBUS, USART_FLAG_TXDE);
        USART_ConfigInt(USART_MODBUS, USART_INT_TXDE, ENABLE);
    } else {
        USART_ConfigInt(USART_MODBUS, USART_INT_TXDE, DISABLE);
    }
}

void vMBPortClose(void)
{
    USART_Enable(USART_MODBUS, DISABLE);
    // RS485_RTS_LOW; // Ensure RTS is low when closing
}

BOOL xMBPortSerialPutByte(CHAR ucByte)
{
    /* Put a byte in the UARTs transmit buffer. This function is called
     * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
     * called. */
    USART_SendData(USART_MODBUS, ucByte);
    return TRUE;
}

BOOL xMBPortSerialGetByte(CHAR* pucByte)
{
    /* Return the byte in the UARTs receive buffer. This function is called
     * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
     */
    *pucByte = modbus_rx_char;
    return TRUE;
}

/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call
 * xMBPortSerialPutByte( ) to send the character.
 */
static void prvvUARTTxReadyISR(void)
{
    pxMBFrameCBTransmitterEmpty();
}

/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
static void prvvUARTRxISR(void)
{
    pxMBFrameCBByteReceived();
}

void UART5_IRQHandler(void)
{
    if (USART_GetIntStatus(USART_MODBUS, USART_INT_RXDNE) != RESET) {
        /* Read one byte from the receive data register */
        modbus_rx_char = USART_ReceiveData(USART_MODBUS);
        prvvUARTRxISR();
        USART_ClrIntPendingBit(USART_MODBUS, USART_INT_RXDNE);
        USART_ClrFlag(USART_MODBUS, USART_FLAG_RXDNE);
    }

    if (USART_GetIntStatus(USART_MODBUS, USART_INT_TXDE) != RESET) {
        /* Write one byte to the transmit data register */
        prvvUARTTxReadyISR();
        USART_ClrIntPendingBit(USART_MODBUS, USART_INT_TXDE);
        USART_ClrFlag(USART_MODBUS, USART_FLAG_TXDE);
    }

    if ((USART_GetFlagStatus(USART_MODBUS, USART_FLAG_OREF) != RESET) ||
        (USART_GetFlagStatus(USART_MODBUS, USART_FLAG_NEF) != RESET) ||
        (USART_GetFlagStatus(USART_MODBUS, USART_FLAG_PEF) != RESET) ||
        (USART_GetFlagStatus(USART_MODBUS, USART_FLAG_FEF) != RESET)) {
        /*Read the sts register first,and the read the DAT register to clear the all error flag*/
        (void)USART_MODBUS->STS;
        (void)USART_MODBUS->DAT;
        /* Under normal circumstances, all error flags will be cleared when the upper data is read and will not be executed here;
           users can add their own processing according to the actual scenario. */
    }
}

#endif