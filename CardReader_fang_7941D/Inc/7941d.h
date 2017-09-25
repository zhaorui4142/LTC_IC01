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

//函数声明
void S7941D_Init(UART_HandleTypeDef* huart, uint16_t* pBuf);//初始化函数
void S7941D_Poll(void);         //轮询函数

void S7941D_UartIdleISR(void);//串口闲时中断服务函数




#ifdef __cplusplus
}
#endif
#endif /*__ pinoutConfig_H */

/****************************END OF FILE****/

