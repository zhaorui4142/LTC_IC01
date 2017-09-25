


/* Includes ------------------------------------------------------------------*/
#include "7941d.h"
#include "led_and_beep.h"


//宏定义区--------------------------------------------------------------------*/
#define RX_BUFFER_SIZE 32
#define DATA_HOLDING_MS 3000
#define BEEP_HOLDING_MS 500
/* ----------------------- Type definitions ---------------------------------*/
//序列号-modbus指针
typedef struct 
{
    uint16_t* state;
    uint16_t* buf;
}Rfid_TagTypeDef;




//函数声明区------------------------------------------------------------------*/

void UpdateModbusBuffer(void);//更新缓冲区数据

bool AnalizeRxBuffer(void);//解析数据是否正确

bool CheckTimeout(uint32_t start_time, uint32_t timeout);//计算是否超时



//全局变量区------------------------------------------------------------------*/

static UART_HandleTypeDef* phuart;
static uint8_t RxBuffer[RX_BUFFER_SIZE];

Rfid_TagTypeDef hcommtag;
Rfid_TagTypeDef hidtag;
Rfid_TagTypeDef hictag;

bool flag_RxCplx;
bool flag_DealyClear;
uint32_t start_time;



//初始化函数，传入huart*，modubs缓冲区指针
void S7941D_Init(UART_HandleTypeDef* huart, uint16_t* pBuf)
{
    flag_RxCplx = false;
    flag_DealyClear = false;
    
    phuart = huart;    
    
    //对应的地址是在modbus中的地址
    hcommtag.state      =   pBuf + 0;
    hcommtag.buf        =   pBuf + 1;
    hidtag.state        =   pBuf + 10;
    hidtag.buf          =   pBuf + 11;
    hictag.state        =   pBuf + 14;
    hictag.buf          =   pBuf + 15;

    //启动接收
    __HAL_UART_ENABLE_IT(phuart, UART_IT_IDLE);//使能空闲中断  
    HAL_UART_Receive_DMA(phuart, RxBuffer, RX_BUFFER_SIZE);
}

//轮询函数,放在主函数循环中
void S7941D_Poll(void)
{
    if(flag_RxCplx)
    {
        //解析RxBuffer
        if(AnalizeRxBuffer())
        {
            //更新modbus寄存器
            UpdateModbusBuffer();
            
            start_time = HAL_GetTick();
            BeepDelayOff(BEEP_HOLDING_MS);
            flag_DealyClear = true;
        }
        
        //重新启动接收
        __HAL_UART_ENABLE_IT(phuart, UART_IT_IDLE);//使能空闲中断  
        HAL_UART_Receive_DMA(phuart, RxBuffer, RX_BUFFER_SIZE);
        
        //置位标记
        flag_RxCplx = false;
    }
    
    if(flag_DealyClear)
    {
        if(CheckTimeout(start_time, DATA_HOLDING_MS))
        {
            //超时清零数据
            hidtag.state[0] = 0x0000;
            hidtag.buf[0] = 0x0000;
            hidtag.buf[1] = 0x0000;
            hidtag.buf[2] = 0x0000;
            hictag.state[0] = 0x0000;
            hictag.buf[0] = 0x0000;
            hictag.buf[1] = 0x0000;
            hictag.buf[2] = 0x0000;
            hcommtag.state[0] = 0x0000;
            hcommtag.buf[0] = 0x0000;
            hcommtag.buf[1] = 0x0000;
            hcommtag.buf[2] = 0x0000;
            
            Led1TurnOff();//LED指示
            Led2TurnOff();//LED指示
            
            flag_DealyClear = false;
        }
    }
}

//更新modbus缓冲区
void UpdateModbusBuffer(void)
{
    uint8_t CardType = RxBuffer[2];
    switch(CardType)
    {
        case 0x02:      //EM4100
        {
            hidtag.state[0] = 0x0001;
            hidtag.buf[0] = 0x0000 | RxBuffer[5];
            hidtag.buf[1] = (((uint16_t)RxBuffer[6])<<8) | RxBuffer[7];
            hidtag.buf[2] = (((uint16_t)RxBuffer[8])<<8) | RxBuffer[9];
            hcommtag.state[0] = 0x0001;
            hcommtag.buf[0] = hidtag.buf[0];
            hcommtag.buf[1] = hidtag.buf[1];
            hcommtag.buf[2] = hidtag.buf[2];
        
            Led1TurnOn();//LED指示
            Led2TurnOff();//LED指示
        }break;
        
        case 0x01:       //MIFARE 1K
        case 0x03:       //MIFARE 4K
        {
            hictag.state[0] = 0x0001;
            hictag.buf[0] = 0x0000;
            hictag.buf[1] = (((uint16_t)RxBuffer[5])<<8) | RxBuffer[6];
            hictag.buf[2] = (((uint16_t)RxBuffer[7])<<8) | RxBuffer[8];
            hcommtag.state[0] = 0x0002;
            hcommtag.buf[0] =  hictag.buf[0];
            hcommtag.buf[1] =  hictag.buf[1];
            hcommtag.buf[2] =  hictag.buf[2];

            Led2TurnOn();//LED指示
        }break;
        
        case 0x10: //HID 卡
        case 0x11: //T5567
        case 0x20: //二代证
        case 0x21: //ISO14443B
        case 0x22: //FELICA
        case 0x30: //15693标签
        case 0x50: //CPU 卡
        case 0x51: //扇区信息
        case 0xFF: //键盘数据
        {
            hcommtag.state[0] = CardType;
            hcommtag.buf[1] = (((uint16_t)RxBuffer[4])<<8) | RxBuffer[5];
            hcommtag.buf[2] = (((uint16_t)RxBuffer[6])<<8) | RxBuffer[7];
        }
    }
    
}



//接收完成中断服务函数，放在串空闲回调中
void S7941D_UartIdleISR(void)
{
    //禁止空闲中断
    __HAL_UART_DISABLE_IT(phuart, UART_IT_IDLE);    
    
    //设置状态机变量
    flag_RxCplx = true;
    
    //清除空闲中断标志
    __HAL_UART_CLEAR_IDLEFLAG(phuart);
    //__HAL_UART_CLEAR_FLAG(phuart, UART_IT_IDLE);
}


//解析接收的数据包
bool AnalizeRxBuffer(void)
{
    //判断协议头是否正确
    if(RxBuffer[0] != 0x02)
        return false;

    //数据长度
    uint8_t len = RxBuffer[1];
    
    //计算异或校验
    uint8_t check=0, i;
    for(i = 1; i <= len-3; i++)
    {
        check ^= RxBuffer[i];
    }
    if(check != RxBuffer[i])
        return false;
    
    //判断数据尾
    if(RxBuffer[len-1] != 0x03)
        return false;
    
    //解析成功
    return true;
}

//内部函数：计算是否超时
bool CheckTimeout(uint32_t start_time, uint32_t timeout)
{
    uint32_t tick = HAL_GetTick();
    uint32_t ellipsed;
    if(tick >= start_time)
        ellipsed =  tick - start_time;
    else
        ellipsed =  (0xFFFFFFFF - start_time) + tick;
    
    if(ellipsed >= timeout)
        return true;
    else
        return false;
}


