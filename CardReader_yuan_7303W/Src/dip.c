


/* Includes ------------------------------------------------------------------*/
#include "dip.h"

//获取SW1的拨码值
uint8_t DIP_GetSW1BitStat(void)
{
    uint8_t retval;
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW1_CH1_GPIO_Port, SW1_CH1_Pin);
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW1_CH2_GPIO_Port, SW1_CH2_Pin);
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW1_CH3_GPIO_Port, SW1_CH3_Pin);
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW1_CH4_GPIO_Port, SW1_CH4_Pin);
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW1_CH5_GPIO_Port, SW1_CH5_Pin);
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW1_CH6_GPIO_Port, SW1_CH6_Pin);
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW1_CH7_GPIO_Port, SW1_CH7_Pin);
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW1_CH8_GPIO_Port, SW1_CH8_Pin);
    return retval ^ 0xff;
}

//获取SW2的拨码值
uint8_t DIP_GetSW2BitStat(void)
{
    uint8_t retval = 0x00;
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW2_CH1_GPIO_Port, SW2_CH1_Pin);
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW2_CH2_GPIO_Port, SW2_CH2_Pin);
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW2_CH3_GPIO_Port, SW2_CH3_Pin);
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW2_CH4_GPIO_Port, SW2_CH4_Pin);
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW2_CH5_GPIO_Port, SW2_CH5_Pin);
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW2_CH6_GPIO_Port, SW2_CH6_Pin);
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW2_CH7_GPIO_Port, SW2_CH7_Pin);
    retval = (retval << 1) | HAL_GPIO_ReadPin(SW2_CH8_GPIO_Port, SW2_CH8_Pin);
    return retval ^ 0xff;
}

//获取从站地址
uint8_t DIP_GetAddress(void)
{
    uint8_t val = DIP_GetSW1BitStat();
    return val;
}

//获取波特率
uint32_t DIP_GetBadurate(void)
{
    uint8_t val = DIP_GetSW2BitStat();
    switch(val >> 4)
    {
        case 0: return 1200;
        case 1: return 2400;
        case 2: return 4800;
        case 3: return 9600;
        case 4: return 19200;
        case 5: return 38400;
        case 6: return 57600;
        case 7: return 115200;
        default: return 9600;
    }
}

//获取校验位
uint8_t DIP_GetParity(void)
{
    uint8_t val = DIP_GetSW2BitStat();
    return ((val & 0x0C) >> 2);
}

//获取数据位长度
uint8_t DIP_GetDataBitsLen(void)
{
    uint8_t val = DIP_GetSW2BitStat();
    if(val & 0x02)
        return 8;
    else
        return 7;
}

//获取停止位长度
uint8_t DIP_GetStopBitsLen(void)
{
    uint8_t val = DIP_GetSW2BitStat();
    if(val & 0x01)
        return 2;
    else
        return 1;
}










