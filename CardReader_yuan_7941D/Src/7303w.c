


/* Includes ------------------------------------------------------------------*/
#include "7303w.h"
#include "led_and_beep.h"


//�궨����--------------------------------------------------------------------*/
#define RX_BUFFER_MAX 32
#define TX_BUFFER_MAX 32
#define SWITCH_DELAY_MS 50
#define RW_RETRY_TIMES 6
#define TAG_ID_RETRY_TIMES 4   //ID�����кŶ�ȡ���Դ���
#define TAG_IC_RETRY_TIMES 4   //IC�����кŶ�ȡ���Դ���
#define STATE_HOLDING_MS 3000
#define TX_RX_TIMEOUT_MS 500
/* ----------------------- Type definitions ---------------------------------*/
//���к�-modbusָ��
typedef struct 
{
    uint16_t* state;
    uint16_t* buf;
    uint32_t state_start_time;
    uint8_t retryCounter;
}Rfid_TagTypeDef;

//��������-modbusָ��
typedef struct 
{
    uint16_t* state;
    uint16_t* command;
    uint16_t* sector;
    uint16_t* block;
    uint16_t* key_type;
    uint16_t* key;
    uint16_t* write_buf; 
    uint16_t* read_buf;
    uint32_t state_start_time;
    uint8_t retryCounter;
}Rfid_SectorTypeDef;

//FSM_������
typedef enum
{
    IC,
    ID
} FSM_CARD_TYPE;

//FSM_���ڽ��еĲ���
typedef enum
{
    READ_TAG,
    READ_DATA,
    WRITE_DATA
} FSM_OPERATE_TYPE;

//FSM_����״̬
typedef enum
{
    OPERATE_STBY,
    SENDING,
    SEND_OVER,
    RECEIVING,
    RECEIVE_OVER,
    OPERATE_OK,
    OPERATE_ERR
} FSM_OPERATE_STATE;

//FSM_״̬��״̬����
typedef struct
{
    FSM_CARD_TYPE card_type;
    FSM_OPERATE_TYPE operate;
    FSM_OPERATE_STATE state;
    bool beepTrigger;//��������
    uint32_t stby_start_time;
    uint32_t TxRx_start_time;
}FSM_HandleTypeDef;

//����������------------------------------------------------------------------*/
void SendTxBuffer(void);//׼�����ͻ�����������
void UpdateModbusBuffer(void);//���»���������
bool AnalizeRxBuffer(void);//���������Ƿ���ȷ
bool CheckTimeout(uint32_t start_time, uint32_t timeout);//�����Ƿ�ʱ



//ȫ�ֱ�����------------------------------------------------------------------*/
FSM_HandleTypeDef fsm;
static UART_HandleTypeDef* phuart;
static uint8_t RxBuffer[RX_BUFFER_MAX];
static uint8_t TxBuffer[TX_BUFFER_MAX];
Rfid_TagTypeDef hcommtag;
Rfid_TagTypeDef hidtag;
Rfid_TagTypeDef hictag;
Rfid_SectorTypeDef hicsector;



//��ʼ������������huart*��modubs������ָ��
void RfidUartInit(UART_HandleTypeDef* huart, uint16_t* pBuf)
{
    phuart = huart;    
    fsm.state = OPERATE_STBY;
    fsm.card_type = ID;
    fsm.operate = READ_TAG;
    fsm.beepTrigger = false;
    fsm.stby_start_time = HAL_GetTick();
    hictag.retryCounter = 0;
    hidtag.retryCounter = 0;
    
    //��Ӧ�ĵ�ַ����modbus�еĵ�ַ
    hcommtag.state      =   pBuf + 0;
    hcommtag.buf        =   pBuf + 1;
    hidtag.state        =   pBuf + 10;
    hidtag.buf          =   pBuf + 11;
    hictag.state        =   pBuf + 14;
    hictag.buf          =   pBuf + 15;
    hicsector.state     =   pBuf + 18;
    hicsector.command   =   pBuf + 19;
    hicsector.sector    =   pBuf + 20;
    hicsector.block     =   pBuf + 21;
    hicsector.key_type  =   pBuf + 22;
    hicsector.key       =   pBuf + 23;
    hicsector.write_buf =   pBuf + 26;
    hicsector.read_buf  =   pBuf + 34;
}

//��ѯ����,����������ѭ����
void RfidPoll(void)
{

    switch(fsm.state)
    {
        //����֡�������
        case SEND_OVER:
        {
            //��������
            __HAL_UART_ENABLE_IT(phuart, UART_IT_IDLE);//ʹ�ܿ����ж�    
            HAL_UART_Receive_DMA(phuart, RxBuffer,  RX_BUFFER_MAX);
            
            //�л�״̬��״̬
            fsm.TxRx_start_time = HAL_GetTick();
            fsm.state = RECEIVING;
        }break;
        
        //�������
        case RECEIVE_OVER:
        {
            //����֡������ȷ
            if(AnalizeRxBuffer() == true)
            {
                //�л�״̬��״̬
                fsm.state = OPERATE_OK;
            }
            //����֡��������
            else
            {
                //�л�״̬��״̬
                fsm.state = OPERATE_ERR;
            }
        }break;
        
        //�����ɹ�
        case OPERATE_OK:
        {
            //ת�����״̬
            fsm.stby_start_time = HAL_GetTick();
            fsm.state = OPERATE_STBY;
        }break;
        
        //��������
        case OPERATE_ERR:
        {
            //ת�����״̬
            fsm.stby_start_time = HAL_GetTick();
            fsm.state = OPERATE_STBY;
        }break;
            
        //����״̬
        case OPERATE_STBY:
        {
            //��ʱһ��ʱ��
            if(CheckTimeout(fsm.stby_start_time, SWITCH_DELAY_MS))
            {
                switch(hicsector.command[0])
                {
                    case 0x0000://��ϵ�к�
                    {
                        //ʵ�ֹ��ܣ�ic����ȡһ���������л���id����ȡ
                        fsm.operate = READ_TAG;
                        //fsm.card_type = (fsm.card_type == ID)?IC:ID;
                        if(fsm.card_type == IC)
                        {
                            if(++hictag.retryCounter >= TAG_IC_RETRY_TIMES)
                            {
                                fsm.card_type = ID;
                                hidtag.retryCounter = 0;
                            }
                                
                        }
                        else
                        {
                            if(++hidtag.retryCounter >= TAG_ID_RETRY_TIMES)
                            {
                                fsm.card_type = IC;
                                hictag.retryCounter = 0;
                            }
                        }
                        
                    }break;
                
                    case 0x0001://����������
                    {
                        fsm.operate = READ_DATA;
                        fsm.card_type = IC;
                    }break;
                
                    case 0x0002://д��������
                    {
                        fsm.operate = WRITE_DATA;
                        fsm.card_type = IC;
                    }break;
                }
                //�����д����
                SendTxBuffer();
                fsm.state = SENDING;
                fsm.TxRx_start_time = HAL_GetTick();
            }
        }break;
        
        //������״̬�ͽ�����״̬
        case SENDING:
        case RECEIVING:
        {
            //����Ƿ�ʱ
            if(CheckTimeout(fsm.TxRx_start_time, TX_RX_TIMEOUT_MS))
            {
                HAL_UART_DMAStop(phuart);
                fsm.state = OPERATE_ERR;
            }
        }
        break;
    }
    
    //����modbus�Ĵ���
    UpdateModbusBuffer();

}

//����modbus������
void UpdateModbusBuffer(void)
{
    //1. ��ȡ��ID���к�
    if(fsm.card_type == ID && fsm.operate == READ_TAG && fsm.state == OPERATE_OK)
    {
        //ID�洢��
        hidtag.state[0] = 0x0001;
        hidtag.buf[0] = 0x0000 | RxBuffer[5];
        hidtag.buf[1] = (((uint16_t)RxBuffer[6])<<8) | RxBuffer[7];
        hidtag.buf[2] = (((uint16_t)RxBuffer[8])<<8) | RxBuffer[9];
            
        hidtag.state_start_time = HAL_GetTick();
        
        Led1TurnOn();//LEDָʾ
    }
    if(hidtag.state[0] != 0x0000)
    {
        if(CheckTimeout(hidtag.state_start_time, STATE_HOLDING_MS))
        {
            hidtag.state[0] = 0x0000;
            hidtag.buf[0] = 0x0000;
            hidtag.buf[1] = 0x0000;
            hidtag.buf[2] = 0x0000;
            
            Led1TurnOff();//LEDָʾ
        }
    }
    
    //2. IC���кŴ洢��
    if(fsm.card_type == IC && fsm.operate == READ_TAG && fsm.state == OPERATE_OK)
    {
        hictag.buf[0] = 0x0000;
        hictag.buf[1] = (((uint16_t)RxBuffer[5])<<8) | RxBuffer[6];
        hictag.buf[2] = (((uint16_t)RxBuffer[7])<<8) | RxBuffer[8];
        
        hictag.state[0] = 0x0001;
        hictag.state_start_time = HAL_GetTick();
        
         Led2TurnOn();//LEDָʾ
    }
    if(hictag.state[0] != 0x0000)
    {
        if(CheckTimeout(hictag.state_start_time, STATE_HOLDING_MS))
        {
            hictag.state[0] = 0;
            hictag.buf[0] = 0x0000;
            hictag.buf[1] = 0x0000;
            hictag.buf[2] = 0x0000;
            
             Led2TurnOff();//LEDָʾ
        }
    }
    
    //3. ͨ�����кŴ洢��
    if((hidtag.state[0] != 0x0000) || (hictag.state[0] != 0x0000))
    {
        if(hidtag.state[0] != 0x0000 && hictag.state[0] == 0x0000)
        {
            hcommtag.state[0] = 0x0001;
            hcommtag.buf[0] = hidtag.buf[0];
            hcommtag.buf[1] = hidtag.buf[1];
            hcommtag.buf[2] = hidtag.buf[2];
        }
        if(hidtag.state[0] == 0x0000 && hictag.state[0] != 0x0000)
        {
            hcommtag.state[0] = 0x0002;
            hcommtag.buf[0] = hictag.buf[0];
            hcommtag.buf[1] = hictag.buf[1];
            hcommtag.buf[2] = hictag.buf[2];
        }
        if(fsm.beepTrigger == false)
        {
            fsm.beepTrigger = true;//����������
            BeepDelayOff(500);
        }
    }
    else
    {
        hcommtag.buf[0] = 0x0000;
        hcommtag.buf[1] = 0x0000;
        hcommtag.buf[2] = 0x0000;
        hcommtag.state[0] = 0x0000;
        if(fsm.beepTrigger == true)
        {
            fsm.beepTrigger = false;//�رշ�����
            
        }
    }
    
    //4. IC����������
    if(fsm.card_type == IC)
    {
        if(fsm.operate == READ_DATA)
        {
            switch(fsm.state)
            {
                case OPERATE_OK://�����ɹ�
                {
                    for(uint8_t i = 0; i<16; i+=2)
                    {hicsector.read_buf[i/2] = (((uint16_t)RxBuffer[7+i])<<8) | RxBuffer[8+i];}
            
                    hicsector.state[0] = 0x0005;//��ȡ�ɹ�
                    hicsector.state_start_time = HAL_GetTick();
                    
                    fsm.beepTrigger = true;//����������
                    BeepDelayOff(500);
            
                    //��д�����Զ�����
                    hicsector.command[0] = 0x0000;
                }break;
        
                case OPERATE_ERR://��������
                {
                    //����������Դ���
                    if(++hicsector.retryCounter >= RW_RETRY_TIMES)
                    {
                        hicsector.state[0] = 0x0006;//��ȡʧ��
                        hicsector.state_start_time = HAL_GetTick();
                        
                        //��д�����Զ�����
                        hicsector.command[0] = 0x0000;
                    }
                }break;
            
                default:
                {
                    hicsector.state[0] = 0x0004;//��ȡ��
                    hicsector.state_start_time = HAL_GetTick();
                }
            }
            
        }
        if(fsm.operate == WRITE_DATA)
        {
            switch(fsm.state)
            {
                case OPERATE_OK://�����ɹ�
                {            
                    hicsector.state[0] = 0x0002;//��ȡ�ɹ�
                    hicsector.state_start_time = HAL_GetTick();
                    
                    fsm.beepTrigger = true;//����������
                    BeepDelayOff(500);
            
                    //��д�����Զ�����
                    hicsector.command[0] = 0x0000;
                }break;
        
                case OPERATE_ERR://��������
                {
                    //����������Դ���
                    if(++hicsector.retryCounter >= RW_RETRY_TIMES)
                    {
                        hicsector.state[0] = 0x0003;//��ȡʧ��
                        hicsector.state_start_time = HAL_GetTick();
                        
                        //��д�����Զ�����
                        hicsector.command[0] = 0x0000;
                    }
                }break;
            
                default:
                {
                    hicsector.state[0] = 0x0001;//��ȡ��
                    hicsector.state_start_time = HAL_GetTick();
                }
            }
        }
    }
    
    if(hicsector.command[0] == 0x0000) 
        hicsector.retryCounter = 0;//����������Լ�����
    
    if(hicsector.state[0] != 0x0000)
    {
        if(CheckTimeout(hicsector.state_start_time, STATE_HOLDING_MS))
        {
            hicsector.state[0] = 0x0000;//��ʱ����
        }
    }
}

//������������ѯ
bool isBeepTrigged(void)
{
    if(fsm.beepTrigger != false)
    {
        fsm.beepTrigger = false;
        return true;
    }
    else
        return false;
}

//״̬��ѯ
RFID_MODE_ENUM  RfidMode(void)
{
    RFID_MODE_ENUM mode = MODE_NONE;
    //IC������
    if(fsm.card_type == IC)
    {
        if(fsm.operate == READ_TAG)
            mode = MODE_IC_READ_TAG;
        if(fsm.operate == READ_DATA)
            mode =  MODE_IC_READ_DATA;
        if(fsm.operate == WRITE_DATA)
            mode = MODE_IC_WRITE_DATA;
    } 
    //ID������
    else
    {
        if(fsm.operate == READ_TAG)
            mode = MODE_ID_READ_TAG;
    }
    return  mode;
}

//��������жϷ����������ڴ��ڷ�����ɻص�HAL_UART_TxCpltCallback��
void RfidUartTxCpltISR(void)
{
    //����״̬������
    if(fsm.state == SENDING) 
        fsm.state = SEND_OVER;
    else
        fsm.state = OPERATE_ERR;
}

//��������жϷ����������ڴ��ڽ�����ɻص�HAL_UART_RxCpltCallback��
void RfidUartRxCpltISR(void)
{
    //��ֹ�����ж�
    __HAL_UART_DISABLE_IT(phuart, UART_IT_IDLE);    
    //����״̬������
    if(fsm.state == RECEIVING) 
        fsm.state = RECEIVE_OVER;
    else
        fsm.state = OPERATE_ERR;
    
    //��������жϱ�־
    __HAL_UART_CLEAR_IDLEFLAG(phuart);
    //__HAL_UART_CLEAR_FLAG(phuart, UART_IT_IDLE);
}


//׼�����ͻ�����������
void SendTxBuffer(void)
{
    uint16_t send_size;
    uint8_t check=0x00,i;
    
    //����ͷ
    TxBuffer[0] = 0xAB;
    TxBuffer[1] = 0xBA;
    TxBuffer[2] = 0x00;
    
    switch(fsm.operate)
    {
        //��ȡ���к�
        case READ_TAG:
        {
            uint8_t cmd = (fsm.card_type == ID)?0x15:0x10;
            TxBuffer[3] = cmd;
            TxBuffer[4] = 0x00;
            send_size = 6;
        }break;
        //��ȡ����
        case READ_DATA:
        {
            TxBuffer[3] = 0x12;//������
            TxBuffer[4] = 0x09;//�����ֽ���
            TxBuffer[5] = hicsector.sector[0];//����
            TxBuffer[6] = hicsector.block[0];//���
            TxBuffer[7] = hicsector.key_type[0];//A�黹��B������
            for(i=0; i<6; i+=2)
            {
                TxBuffer[8+i] = hicsector.key[i/2]>>8;
                TxBuffer[9+i] = hicsector.key[i/2];
            }
            send_size = 15;
        }break;
        //д������
        case WRITE_DATA:
        {
            TxBuffer[3] = 0x13;//������
            TxBuffer[4] = 0x19;//�����ֽ���
            TxBuffer[5] = hicsector.sector[0];//����
            TxBuffer[6] = hicsector.block[0];//���
            TxBuffer[7] = hicsector.key_type[0];//A�黹��B������
            for(i=0; i<6; i+=2)
            {
                TxBuffer[8+i] = hicsector.key[i/2]>>8;
                TxBuffer[9+i] = hicsector.key[i/2];
            }        
            for(i=0; i<16; i+=2)
            {
                TxBuffer[14+i] = hicsector.write_buf[i/2]>>8;
                TxBuffer[15+i] = hicsector.write_buf[i/2];
            }
            send_size = 31;
        }break;  
    }
    
    //�������У��
    for(i = 2; i<send_size-1; i++)
    {
        check ^= TxBuffer[i];
    }
    TxBuffer[send_size-1] = check;//���У��
    
    //��������
    HAL_UART_Transmit_DMA(phuart, TxBuffer, send_size);
}

/*
//ȡ�����ջ�����������
void TakeRxBuffer(void)
{
    int i;
    switch(fsm.operate)
    {
        //��ȡ���к�
        case READ_TAG:
        {
            uint16_t tag[3];
            //ID��
            if(fsm.card_type == ID)
            {
                tag[0] = 0x0000 | RxBuffer[5];
                tag[1] = (((uint16_t)RxBuffer[6])<<8) | RxBuffer[7];
                tag[2] = (((uint16_t)RxBuffer[8])<<8) | RxBuffer[9];
                hidtag.state[0] = 0x0001;
                hidtag.state_start_time = HAL_GetTick();
                for(i=0; i<3; i++) hidtag.buf[i] = tag[i];
            }
            //IC��
            else
            {
                tag[0] = 0x0000;
                tag[1] = (((uint16_t)RxBuffer[5])<<8) | RxBuffer[6];
                tag[2] = (((uint16_t)RxBuffer[7])<<8) | RxBuffer[8];
                hictag.state[0] = 0x0001;
                hictag.state_start_time = HAL_GetTick();
                for(i=0; i<3; i++) hidtag.buf[i] = tag[i];
            }
                
            //���common��û�����ݣ���д��
            if(hcommtag.state == 0x0000)
            {
                hcommtag.state[0] = 0x0001;
                hcommtag.state_start_time = HAL_GetTick();
                hcommtag.buf[0] = 0x0000;
                hcommtag.buf[1] = (((uint16_t)RxBuffer[6])<<8) | RxBuffer[7];
                hcommtag.buf[2] = (((uint16_t)RxBuffer[8])<<8) | RxBuffer[9];
                fsm.beepTrigger = true;//����������
            }
        }break;
        //��ȡ����
        case READ_DATA:
        {
            for(uint8_t i = 0; i<16; i+=2)
            {hicsector.read_buf[i/2] = (((uint16_t)RxBuffer[7+i])<<8) | RxBuffer[8+i];}
            hicsector.state[0] = 0x0005;//״̬��־
            fsm.beepTrigger = true;//����������
        }break;  
        //д������
        case WRITE_DATA:
        {
            //ʲô������
        }break;  
    }
}*/


//�������յ����ݰ�
bool AnalizeRxBuffer(void)
{
    //�ж�Э��ͷ�Ƿ���ȷ
    if(RxBuffer[0] != 0xCD || RxBuffer[1] != 0xDC)
        return false;

    //�жϷ��������Ƿ���ȷ0x80����0x81��ȷ
    if(RxBuffer[3] != 0x81)
        return false;
    
    //�������У��
    uint8_t check=0, i;
    for(i = 3; i<RxBuffer[4]+5; i++)
    {
        check ^= RxBuffer[i];
    }
    if(check != RxBuffer[i])
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


