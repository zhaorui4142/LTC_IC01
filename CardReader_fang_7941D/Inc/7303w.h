/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __7303W_H
#define __7303W_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "main.h"
#include "stdbool.h"

     
/* ----------------------- Type definitions ---------------------------------*/

typedef enum
{
    MODE_NONE,
    MODE_IC_READ_TAG,
    MODE_IC_READ_DATA,
    MODE_IC_WRITE_DATA,
    MODE_ID_READ_TAG
} RFID_MODE_ENUM;



//��������
void RfidUartInit(UART_HandleTypeDef* huart, uint16_t* pBuf);//��ʼ������
void RfidPoll(void);//��ѯ����
RFID_MODE_ENUM  RfidMode(void);//״̬��ѯ
bool isBeepTrigged(void);//�¿���ѯ
void RfidUartTxCpltISR(void);//��������жϷ�����
void RfidUartRxCpltISR(void);//��������жϷ�����




#ifdef __cplusplus
}
#endif
#endif /*__ pinoutConfig_H */

/****************************END OF FILE****/

