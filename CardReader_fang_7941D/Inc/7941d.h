/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __7941D_H
#define __7941D_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "main.h"
#include "stdbool.h"

     
/* ----------------------- Type definitions ---------------------------------*/

//��������
void S7941D_Init(UART_HandleTypeDef* huart, uint16_t* pBuf);//��ʼ������
void S7941D_Poll(void);         //��ѯ����

void S7941D_UartIdleISR(void);//������ʱ�жϷ�����




#ifdef __cplusplus
}
#endif
#endif /*__ pinoutConfig_H */

/****************************END OF FILE****/

