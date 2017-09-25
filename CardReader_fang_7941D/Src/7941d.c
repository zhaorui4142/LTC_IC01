


/* Includes ------------------------------------------------------------------*/
#include "7941d.h"
#include "led_and_beep.h"


//�궨����--------------------------------------------------------------------*/
#define RX_BUFFER_SIZE 32
#define DATA_HOLDING_MS 3000
#define BEEP_HOLDING_MS 500
/* ----------------------- Type definitions ---------------------------------*/
//���к�-modbusָ��
typedef struct 
{
    uint16_t* state;
    uint16_t* buf;
}Rfid_TagTypeDef;




//����������------------------------------------------------------------------*/

void UpdateModbusBuffer(void);//���»���������

bool AnalizeRxBuffer(void);//���������Ƿ���ȷ

bool CheckTimeout(uint32_t start_time, uint32_t timeout);//�����Ƿ�ʱ



//ȫ�ֱ�����------------------------------------------------------------------*/

static UART_HandleTypeDef* phuart;
static uint8_t RxBuffer[RX_BUFFER_SIZE];

Rfid_TagTypeDef hcommtag;
Rfid_TagTypeDef hidtag;
Rfid_TagTypeDef hictag;

bool flag_RxCplx;
bool flag_DealyClear;
uint32_t start_time;



//��ʼ������������huart*��modubs������ָ��
void S7941D_Init(UART_HandleTypeDef* huart, uint16_t* pBuf)
{
    flag_RxCplx = false;
    flag_DealyClear = false;
    
    phuart = huart;    
    
    //��Ӧ�ĵ�ַ����modbus�еĵ�ַ
    hcommtag.state      =   pBuf + 0;
    hcommtag.buf        =   pBuf + 1;
    hidtag.state        =   pBuf + 10;
    hidtag.buf          =   pBuf + 11;
    hictag.state        =   pBuf + 14;
    hictag.buf          =   pBuf + 15;

    //��������
    __HAL_UART_ENABLE_IT(phuart, UART_IT_IDLE);//ʹ�ܿ����ж�  
    HAL_UART_Receive_DMA(phuart, RxBuffer, RX_BUFFER_SIZE);
}

//��ѯ����,����������ѭ����
void S7941D_Poll(void)
{
    if(flag_RxCplx)
    {
        //����RxBuffer
        if(AnalizeRxBuffer())
        {
            //����modbus�Ĵ���
            UpdateModbusBuffer();
            
            start_time = HAL_GetTick();
            BeepDelayOff(BEEP_HOLDING_MS);
            flag_DealyClear = true;
        }
        
        //������������
        __HAL_UART_ENABLE_IT(phuart, UART_IT_IDLE);//ʹ�ܿ����ж�  
        HAL_UART_Receive_DMA(phuart, RxBuffer, RX_BUFFER_SIZE);
        
        //��λ���
        flag_RxCplx = false;
    }
    
    if(flag_DealyClear)
    {
        if(CheckTimeout(start_time, DATA_HOLDING_MS))
        {
            //��ʱ��������
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
            
            Led1TurnOff();//LEDָʾ
            Led2TurnOff();//LEDָʾ
            
            flag_DealyClear = false;
        }
    }
}

//����modbus������
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
        
            Led1TurnOn();//LEDָʾ
            Led2TurnOff();//LEDָʾ
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

            Led2TurnOn();//LEDָʾ
        }break;
        
        case 0x10: //HID ��
        case 0x11: //T5567
        case 0x20: //����֤
        case 0x21: //ISO14443B
        case 0x22: //FELICA
        case 0x30: //15693��ǩ
        case 0x50: //CPU ��
        case 0x51: //������Ϣ
        case 0xFF: //��������
        {
            hcommtag.state[0] = CardType;
            hcommtag.buf[1] = (((uint16_t)RxBuffer[4])<<8) | RxBuffer[5];
            hcommtag.buf[2] = (((uint16_t)RxBuffer[6])<<8) | RxBuffer[7];
        }
    }
    
}



//��������жϷ����������ڴ����лص���
void S7941D_UartIdleISR(void)
{
    //��ֹ�����ж�
    __HAL_UART_DISABLE_IT(phuart, UART_IT_IDLE);    
    
    //����״̬������
    flag_RxCplx = true;
    
    //��������жϱ�־
    __HAL_UART_CLEAR_IDLEFLAG(phuart);
    //__HAL_UART_CLEAR_FLAG(phuart, UART_IT_IDLE);
}


//�������յ����ݰ�
bool AnalizeRxBuffer(void)
{
    //�ж�Э��ͷ�Ƿ���ȷ
    if(RxBuffer[0] != 0x02)
        return false;

    //���ݳ���
    uint8_t len = RxBuffer[1];
    
    //�������У��
    uint8_t check=0, i;
    for(i = 1; i <= len-3; i++)
    {
        check ^= RxBuffer[i];
    }
    if(check != RxBuffer[i])
        return false;
    
    //�ж�����β
    if(RxBuffer[len-1] != 0x03)
        return false;
    
    //�����ɹ�
    return true;
}

//�ڲ������������Ƿ�ʱ
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


