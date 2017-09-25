/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
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
 * File: $Id: portserial.c,v 1.1 2006/08/22 21:35:13 wolti Exp $
 *
 */

#include "port.h"
#include "stm32f1xx_hal.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include "tim.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- static functions ---------------------------------*/
//void prvvUARTTxReadyISR( void );
//void prvvUARTRxISR( void );

//˽�б���
static uint8_t UatrReceiveByte;
/* ----------------------- Start implementation -----------------------------*/
void vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
    /* If xRXEnable enable serial receive interrupts. If xTxENable enable
     * transmitter empty interrupts.
     */
	if (xRxEnable)
	{
        //�˴�Ӧʹ�ܴ��ڽ��ռ������ж�,����һ���ֽڵ����ݣ�����������ж�
		HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
        HAL_UART_Receive_DMA(&huart1 , &UatrReceiveByte, 1);
	}
	else
	{
		HAL_NVIC_DisableIRQ(DMA1_Channel5_IRQn);
	}
	if (xTxEnable)
	{
        //�˴�Ӧʹ�ܴ��ڷ��ͼ������ж�
        __HAL_DMA_CLEAR_FLAG(huart1.hdmatx, __HAL_DMA_GET_TC_FLAG_INDEX(huart1.hdmatx));
        HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
        pxMBFrameCBTransmitterEmpty(  );
	}
	else
	{
        HAL_NVIC_DisableIRQ(DMA1_Channel4_IRQn);
	}
}

//���ڳ�ʼ��
BOOL xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
    huart1.Instance = USART1;
    //������
    huart1.Init.BaudRate = (uint32_t)ulBaudRate;

    //huart1.Init.WordLength = (uint32_t)UART_WORDLENGTH_8B;
    //huart1.Init.StopBits = UART_STOPBITS_1;
    
    //У��λ
    if(eParity == MB_PAR_NONE) huart1.Init.Parity = UART_PARITY_NONE;
    if(eParity == MB_PAR_ODD)  huart1.Init.Parity = UART_PARITY_ODD;
    if(eParity == MB_PAR_EVEN) huart1.Init.Parity = UART_PARITY_EVEN;
    
    //����ΪĬ��
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
	return TRUE ;
}

//����һ���ֽ�����
BOOL xMBPortSerialPutByte( CHAR ucByte )
{
  /* Put a byte in the UARTs transmit buffer. This function is called
   * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
   * called. */
  if(HAL_UART_Transmit_DMA(&huart1, (uint8_t *)&ucByte, 1) != HAL_OK )
    return FALSE ;
  else
    return TRUE;
}

//����һ���ֽ�����
BOOL xMBPortSerialGetByte( CHAR * pucByte )
{  
  /* Return the byte in the UARTs receive buffer. This function is called
   * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
   */
    //��ȡ����
    *pucByte = UatrReceiveByte;
    
    //������������
    if(HAL_UART_Receive_DMA(&huart1 , &UatrReceiveByte, 1) != HAL_OK )
     return FALSE ;
    else
      return TRUE;
}

/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
void prvvUARTTxReadyISR( void )
{
    
  pxMBFrameCBTransmitterEmpty(  );
}

/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
void prvvUARTRxISR( void )
{
  pxMBFrameCBByteReceived(  );
}






