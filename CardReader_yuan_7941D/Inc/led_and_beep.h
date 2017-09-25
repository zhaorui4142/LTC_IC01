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


//函数声明
void LedBeepInit(void);//初始化函数
void LedBeepPoll(void);//轮询函数

void Led1TurnOn(void);//Led1常亮
void Led1TurnOff(void);//Led1关闭
void Led1Blink(uint16_t ms);//Led1闪烁
void Led2TurnOn(void);//Led2常亮
void Led2TurnOff(void);//Led2关闭
void Led2Blink(uint16_t ms);//Led2闪烁

void BeepTurnOn(void);
void BeepTurnOff(void);
void BeepDelayOff(uint16_t ms);//蜂鸣器常响
void BeepDiDi(uint8_t times);//蜂鸣器滴滴


#ifdef __cplusplus
}
#endif
#endif /*__ pinoutConfig_H */

/****************************END OF FILE****/

