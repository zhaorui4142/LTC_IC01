/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LED_AND_BEEP_H
#define __LED_AND_BEEP_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "main.h"

     
/* ----------------------- Type definitions ---------------------------------*/


//��������
void LedBeepInit(void);//��ʼ������
void LedBeepPoll(void);//��ѯ����

void Led1TurnOn(void);//Led1����
void Led1TurnOff(void);//Led1�ر�
void Led1Blink(uint16_t ms);//Led1��˸
void Led2TurnOn(void);//Led2����
void Led2TurnOff(void);//Led2�ر�
void Led2Blink(uint16_t ms);//Led2��˸

void BeepTurnOn(void);
void BeepTurnOff(void);
void BeepDelayOff(uint16_t ms);//����������
void BeepDiDi(uint8_t times);//�������ε�


#ifdef __cplusplus
}
#endif
#endif /*__ pinoutConfig_H */

/****************************END OF FILE****/

