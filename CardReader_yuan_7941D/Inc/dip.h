/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __dip_H
#define __dip_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "main.h"

//º¯ÊýÉùÃ÷
uint8_t DIP_GetSW1BitStat(void);
uint8_t DIP_GetSW2BitStat(void);
uint8_t DIP_GetAddress(void);
uint32_t DIP_GetBadurate(void);
uint8_t DIP_GetParity(void);
uint8_t DIP_GetDataBitsLen(void);
uint8_t DIP_GetStopBitsLen(void);

#ifdef __cplusplus
}
#endif
#endif /*__ pinoutConfig_H */

/****************************END OF FILE****/

