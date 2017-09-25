


/* Includes ------------------------------------------------------------------*/
#include "7303w.h"
#include "led_and_beep.h"


//宏定义区--------------------------------------------------------------------*/
#define RX_BUFFER_MAX 32
#define TX_BUFFER_MAX 32
#define SWITCH_DELAY_MS 50
#define RW_RETRY_TIMES 6
#define TAG_ID_RETRY_TIMES 4   //ID卡序列号读取重试次数
#define TAG_IC_RETRY_TIMES 4   //IC卡序列号读取重试次数
#define STATE_HOLDING_MS 3000
#define TX_RX_TIMEOUT_MS 500
/* ----------------------- Type definitions ---------------------------------*/
//序列号-modbus指针
typedef struct 
{
    uint16_t* state;
    uint16_t* buf;
    uint32_t state_start_time;
    uint8_t retryCounter;
}Rfid_TagTypeDef;

//扇区数据-modbus指针
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

//FSM_卡类型
typedef enum
{
    IC,
    ID
} FSM_CARD_TYPE;

//FSM_正在进行的操作
typedef enum
{
    READ_TAG,
    READ_DATA,
    WRITE_DATA
} FSM_OPERATE_TYPE;

//FSM_操作状态
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

//FSM_状态机状态集合
typedef struct
{
    FSM_CARD_TYPE card_type;
    FSM_OPERATE_TYPE operate;
    FSM_OPERATE_STATE state;
    bool beepTrigger;//触发警报
    uint32_t stby_start_time;
    uint32_t TxRx_start_time;
}FSM_HandleTypeDef;

//函数声明区------------------------------------------------------------------*/
void SendTxBuffer(void);//准备发送缓冲区的数据
void UpdateModbusBuffer(void);//更新缓冲区数据
bool AnalizeRxBuffer(void);//解析数据是否正确
bool CheckTimeout(uint32_t start_time, uint32_t timeout);//计算是否超时



//全局变量区------------------------------------------------------------------*/
FSM_HandleTypeDef fsm;
static UART_HandleTypeDef* phuart;
static uint8_t RxBuffer[RX_BUFFER_MAX];
static uint8_t TxBuffer[TX_BUFFER_MAX];
Rfid_TagTypeDef hcommtag;
Rfid_TagTypeDef hidtag;
Rfid_TagTypeDef hictag;
Rfid_SectorTypeDef hicsector;



//初始化函数，传入huart*，modubs缓冲区指针
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
    
    //对应的地址是在modbus中的地址
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

//轮询函数,放在主函数循环中
void RfidPoll(void)
{

    switch(fsm.state)
    {
        //请求帧发送完毕
        case SEND_OVER:
        {
            //启动接收
            __HAL_UART_ENABLE_IT(phuart, UART_IT_IDLE);//使能空闲中断    
            HAL_UART_Receive_DMA(phuart, RxBuffer,  RX_BUFFER_MAX);
            
            //切换状态机状态
            fsm.TxRx_start_time = HAL_GetTick();
            fsm.state = RECEIVING;
        }break;
        
        //接收完毕
        case RECEIVE_OVER:
        {
            //接收帧解析正确
            if(AnalizeRxBuffer() == true)
            {
                //切换状态机状态
                fsm.state = OPERATE_OK;
            }
            //接收帧解析错误
            else
            {
                //切换状态机状态
                fsm.state = OPERATE_ERR;
            }
        }break;
        
        //操作成功
        case OPERATE_OK:
        {
            //转入待机状态
            fsm.stby_start_time = HAL_GetTick();
            fsm.state = OPERATE_STBY;
        }break;
        
        //操作错误
        case OPERATE_ERR:
        {
            //转入待机状态
            fsm.stby_start_time = HAL_GetTick();
            fsm.state = OPERATE_STBY;
        }break;
            
        //待机状态
        case OPERATE_STBY:
        {
            //延时一段时间
            if(CheckTimeout(fsm.stby_start_time, SWITCH_DELAY_MS))
            {
                switch(hicsector.command[0])
                {
                    case 0x0000://读系列号
                    {
                        //实现功能，ic卡读取一定次数后切换到id卡读取
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
                
                    case 0x0001://读扇区数据
                    {
                        fsm.operate = READ_DATA;
                        fsm.card_type = IC;
                    }break;
                
                    case 0x0002://写扇区数据
                    {
                        fsm.operate = WRITE_DATA;
                        fsm.card_type = IC;
                    }break;
                }
                //发起读写请求
                SendTxBuffer();
                fsm.state = SENDING;
                fsm.TxRx_start_time = HAL_GetTick();
            }
        }break;
        
        //发送中状态和接收中状态
        case SENDING:
        case RECEIVING:
        {
            //检测是否超时
            if(CheckTimeout(fsm.TxRx_start_time, TX_RX_TIMEOUT_MS))
            {
                HAL_UART_DMAStop(phuart);
                fsm.state = OPERATE_ERR;
            }
        }
        break;
    }
    
    //更新modbus寄存器
    UpdateModbusBuffer();

}

//更新modbus缓冲区
void UpdateModbusBuffer(void)
{
    //1. 读取到ID序列号
    if(fsm.card_type == ID && fsm.operate == READ_TAG && fsm.state == OPERATE_OK)
    {
        //ID存储区
        hidtag.state[0] = 0x0001;
        hidtag.buf[0] = 0x0000 | RxBuffer[5];
        hidtag.buf[1] = (((uint16_t)RxBuffer[6])<<8) | RxBuffer[7];
        hidtag.buf[2] = (((uint16_t)RxBuffer[8])<<8) | RxBuffer[9];
            
        hidtag.state_start_time = HAL_GetTick();
        
        Led1TurnOn();//LED指示
    }
    if(hidtag.state[0] != 0x0000)
    {
        if(CheckTimeout(hidtag.state_start_time, STATE_HOLDING_MS))
        {
            hidtag.state[0] = 0x0000;
            hidtag.buf[0] = 0x0000;
            hidtag.buf[1] = 0x0000;
            hidtag.buf[2] = 0x0000;
            
            Led1TurnOff();//LED指示
        }
    }
    
    //2. IC序列号存储区
    if(fsm.card_type == IC && fsm.operate == READ_TAG && fsm.state == OPERATE_OK)
    {
        hictag.buf[0] = 0x0000;
        hictag.buf[1] = (((uint16_t)RxBuffer[5])<<8) | RxBuffer[6];
        hictag.buf[2] = (((uint16_t)RxBuffer[7])<<8) | RxBuffer[8];
        
        hictag.state[0] = 0x0001;
        hictag.state_start_time = HAL_GetTick();
        
         Led2TurnOn();//LED指示
    }
    if(hictag.state[0] != 0x0000)
    {
        if(CheckTimeout(hictag.state_start_time, STATE_HOLDING_MS))
        {
            hictag.state[0] = 0;
            hictag.buf[0] = 0x0000;
            hictag.buf[1] = 0x0000;
            hictag.buf[2] = 0x0000;
            
             Led2TurnOff();//LED指示
        }
    }
    
    //3. 通用序列号存储区
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
            fsm.beepTrigger = true;//触发蜂鸣器
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
            fsm.beepTrigger = false;//关闭蜂鸣器
            
        }
    }
    
    //4. IC卡扇区操作
    if(fsm.card_type == IC)
    {
        if(fsm.operate == READ_DATA)
        {
            switch(fsm.state)
            {
                case OPERATE_OK://操作成功
                {
                    for(uint8_t i = 0; i<16; i+=2)
                    {hicsector.read_buf[i/2] = (((uint16_t)RxBuffer[7+i])<<8) | RxBuffer[8+i];}
            
                    hicsector.state[0] = 0x0005;//读取成功
                    hicsector.state_start_time = HAL_GetTick();
                    
                    fsm.beepTrigger = true;//触发蜂鸣器
                    BeepDelayOff(500);
            
                    //读写命令自动清零
                    hicsector.command[0] = 0x0000;
                }break;
        
                case OPERATE_ERR://操作错误
                {
                    //超过最大重试次数
                    if(++hicsector.retryCounter >= RW_RETRY_TIMES)
                    {
                        hicsector.state[0] = 0x0006;//读取失败
                        hicsector.state_start_time = HAL_GetTick();
                        
                        //读写命令自动清零
                        hicsector.command[0] = 0x0000;
                    }
                }break;
            
                default:
                {
                    hicsector.state[0] = 0x0004;//读取中
                    hicsector.state_start_time = HAL_GetTick();
                }
            }
            
        }
        if(fsm.operate == WRITE_DATA)
        {
            switch(fsm.state)
            {
                case OPERATE_OK://操作成功
                {            
                    hicsector.state[0] = 0x0002;//读取成功
                    hicsector.state_start_time = HAL_GetTick();
                    
                    fsm.beepTrigger = true;//触发蜂鸣器
                    BeepDelayOff(500);
            
                    //读写命令自动清零
                    hicsector.command[0] = 0x0000;
                }break;
        
                case OPERATE_ERR://操作错误
                {
                    //超过最大重试次数
                    if(++hicsector.retryCounter >= RW_RETRY_TIMES)
                    {
                        hicsector.state[0] = 0x0003;//读取失败
                        hicsector.state_start_time = HAL_GetTick();
                        
                        //读写命令自动清零
                        hicsector.command[0] = 0x0000;
                    }
                }break;
            
                default:
                {
                    hicsector.state[0] = 0x0001;//读取中
                    hicsector.state_start_time = HAL_GetTick();
                }
            }
        }
    }
    
    if(hicsector.command[0] == 0x0000) 
        hicsector.retryCounter = 0;//清零错误重试计数器
    
    if(hicsector.state[0] != 0x0000)
    {
        if(CheckTimeout(hicsector.state_start_time, STATE_HOLDING_MS))
        {
            hicsector.state[0] = 0x0000;//延时清零
        }
    }
}

//报警器触发查询
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

//状态查询
RFID_MODE_ENUM  RfidMode(void)
{
    RFID_MODE_ENUM mode = MODE_NONE;
    //IC卡操作
    if(fsm.card_type == IC)
    {
        if(fsm.operate == READ_TAG)
            mode = MODE_IC_READ_TAG;
        if(fsm.operate == READ_DATA)
            mode =  MODE_IC_READ_DATA;
        if(fsm.operate == WRITE_DATA)
            mode = MODE_IC_WRITE_DATA;
    } 
    //ID卡操作
    else
    {
        if(fsm.operate == READ_TAG)
            mode = MODE_ID_READ_TAG;
    }
    return  mode;
}

//发送完成中断服务函数，放在串口发送完成回调HAL_UART_TxCpltCallback中
void RfidUartTxCpltISR(void)
{
    //设置状态机变量
    if(fsm.state == SENDING) 
        fsm.state = SEND_OVER;
    else
        fsm.state = OPERATE_ERR;
}

//接收完成中断服务函数，放在串口接收完成回调HAL_UART_RxCpltCallback中
void RfidUartRxCpltISR(void)
{
    //禁止空闲中断
    __HAL_UART_DISABLE_IT(phuart, UART_IT_IDLE);    
    //设置状态机变量
    if(fsm.state == RECEIVING) 
        fsm.state = RECEIVE_OVER;
    else
        fsm.state = OPERATE_ERR;
    
    //清除空闲中断标志
    __HAL_UART_CLEAR_IDLEFLAG(phuart);
    //__HAL_UART_CLEAR_FLAG(phuart, UART_IT_IDLE);
}


//准备发送缓冲区的数据
void SendTxBuffer(void)
{
    uint16_t send_size;
    uint8_t check=0x00,i;
    
    //数据头
    TxBuffer[0] = 0xAB;
    TxBuffer[1] = 0xBA;
    TxBuffer[2] = 0x00;
    
    switch(fsm.operate)
    {
        //读取序列号
        case READ_TAG:
        {
            uint8_t cmd = (fsm.card_type == ID)?0x15:0x10;
            TxBuffer[3] = cmd;
            TxBuffer[4] = 0x00;
            send_size = 6;
        }break;
        //读取数据
        case READ_DATA:
        {
            TxBuffer[3] = 0x12;//功能码
            TxBuffer[4] = 0x09;//发送字节数
            TxBuffer[5] = hicsector.sector[0];//扇区
            TxBuffer[6] = hicsector.block[0];//块号
            TxBuffer[7] = hicsector.key_type[0];//A组还是B组密码
            for(i=0; i<6; i+=2)
            {
                TxBuffer[8+i] = hicsector.key[i/2]>>8;
                TxBuffer[9+i] = hicsector.key[i/2];
            }
            send_size = 15;
        }break;
        //写入数据
        case WRITE_DATA:
        {
            TxBuffer[3] = 0x13;//功能码
            TxBuffer[4] = 0x19;//发送字节数
            TxBuffer[5] = hicsector.sector[0];//扇区
            TxBuffer[6] = hicsector.block[0];//块号
            TxBuffer[7] = hicsector.key_type[0];//A组还是B组密码
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
    
    //计算异或校验
    for(i = 2; i<send_size-1; i++)
    {
        check ^= TxBuffer[i];
    }
    TxBuffer[send_size-1] = check;//异或校验
    
    //发送数据
    HAL_UART_Transmit_DMA(phuart, TxBuffer, send_size);
}

/*
//取出接收缓冲区的数据
void TakeRxBuffer(void)
{
    int i;
    switch(fsm.operate)
    {
        //读取序列号
        case READ_TAG:
        {
            uint16_t tag[3];
            //ID卡
            if(fsm.card_type == ID)
            {
                tag[0] = 0x0000 | RxBuffer[5];
                tag[1] = (((uint16_t)RxBuffer[6])<<8) | RxBuffer[7];
                tag[2] = (((uint16_t)RxBuffer[8])<<8) | RxBuffer[9];
                hidtag.state[0] = 0x0001;
                hidtag.state_start_time = HAL_GetTick();
                for(i=0; i<3; i++) hidtag.buf[i] = tag[i];
            }
            //IC卡
            else
            {
                tag[0] = 0x0000;
                tag[1] = (((uint16_t)RxBuffer[5])<<8) | RxBuffer[6];
                tag[2] = (((uint16_t)RxBuffer[7])<<8) | RxBuffer[8];
                hictag.state[0] = 0x0001;
                hictag.state_start_time = HAL_GetTick();
                for(i=0; i<3; i++) hidtag.buf[i] = tag[i];
            }
                
            //如果common区没有数据，则写入
            if(hcommtag.state == 0x0000)
            {
                hcommtag.state[0] = 0x0001;
                hcommtag.state_start_time = HAL_GetTick();
                hcommtag.buf[0] = 0x0000;
                hcommtag.buf[1] = (((uint16_t)RxBuffer[6])<<8) | RxBuffer[7];
                hcommtag.buf[2] = (((uint16_t)RxBuffer[8])<<8) | RxBuffer[9];
                fsm.beepTrigger = true;//触发蜂鸣器
            }
        }break;
        //读取数据
        case READ_DATA:
        {
            for(uint8_t i = 0; i<16; i+=2)
            {hicsector.read_buf[i/2] = (((uint16_t)RxBuffer[7+i])<<8) | RxBuffer[8+i];}
            hicsector.state[0] = 0x0005;//状态标志
            fsm.beepTrigger = true;//触发蜂鸣器
        }break;  
        //写入数据
        case WRITE_DATA:
        {
            //什么都不做
        }break;  
    }
}*/


//解析接收的数据包
bool AnalizeRxBuffer(void)
{
    //判断协议头是否正确
    if(RxBuffer[0] != 0xCD || RxBuffer[1] != 0xDC)
        return false;

    //判断返回命令是否正确0x80错误，0x81正确
    if(RxBuffer[3] != 0x81)
        return false;
    
    //计算异或校验
    uint8_t check=0, i;
    for(i = 3; i<RxBuffer[4]+5; i++)
    {
        check ^= RxBuffer[i];
    }
    if(check != RxBuffer[i])
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


