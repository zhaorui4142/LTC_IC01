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



//函数声明
void RfidUartInit(UART_HandleTypeDef* huart, uint16_t* pBuf);//初始化函数
void RfidPoll(void);//轮询函数
RFID_MODE_ENUM  RfidMode(void);//状态查询
bool isBeepTrigged(void);//新卡查询
void RfidUartTxCpltISR(void);//发送完成中断服务函数
void RfidUartRxCpltISR(void);//接收完成中断服务函数




#ifdef __cplusplus
}
#endif
#endif /*__ pinoutConfig_H */

/****************************END OF FILE****/

