
/* Includes ------------------------------------------------------------------*/
#include "led_and_beep.h"
#include "tim.h"

//�궨����
#define LED1_BLINK_T1_MS 50
#define LED2_BLINK_T1_MS 50
#define LED1_BLINK_T2_MS 500
#define LED2_BLINK_T2_MS 500
#define BEEP_TICK_ON_MS 100
#define BEEP_TICK_OFF_MS 200

//�Զ���ö������
typedef enum
{
    LED_ON,
    LED_OFF,
    LED_BLINK
} LED_MODE;

typedef enum
{
    BEEP_ON,
    BEEP_OFF,
    BEEP_TICK_ON,
    BEEP_TICK_OFF,
    BEEP_DELAY
} BEEP_MODE;

//ȫ�ֱ���
static LED_MODE mode_led1;
static LED_MODE mode_led2;
static BEEP_MODE mode_beep;
static int led1_blink_start_time;
static int led2_blink_start_time;
static int led1_blink_set_time;
static int led2_blink_set_time;
static int beep_set_time;
static int beep_start_time;
static int beep_set_tickes;
TIM_OC_InitTypeDef sConfigOC;

//�ڲ�����������
uint32_t GetEllipsedTime(uint32_t start_time);

//��ʼ������
void LedBeepInit(void)
{
    led1_blink_start_time = 0;
    led2_blink_start_time = 0;
    led1_blink_set_time = 0;
    led2_blink_set_time = 0;
    beep_set_tickes = 0;
    beep_start_time = 0;
    beep_set_time = 0;
    mode_led1 = LED_OFF;
    mode_led2 = LED_OFF;
    mode_beep = BEEP_OFF;
}

//��ѯ����
void LedBeepPoll(void)
{
    int ellipsed_time;
        
    //LED1����
    switch(mode_led1)
    {
        //����
        case LED_ON:
        {
            HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        }break;
        
        //Ϩ��
        case LED_OFF:
        {
            HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        }break;
        
        //��˸
        case LED_BLINK:
        {
            //���㵱ǰʱ��
            ellipsed_time = GetEllipsedTime(led1_blink_start_time);
            
            //�����趨ʱ��
            if(ellipsed_time >= led1_blink_set_time)
            {
                //��LED
                mode_led1 = LED_OFF;
            }
            //δ���趨ʱ��
            else
            {
                ellipsed_time %= (LED1_BLINK_T1_MS + LED1_BLINK_T2_MS);
                if(ellipsed_time <= LED1_BLINK_T1_MS)
                    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
                else
                    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
            }
        }
    }
    
    //LED2����
    switch(mode_led2)
    {
        //����
        case LED_ON:
        {
            HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
        }break;
        
        //Ϩ��
        case LED_OFF:
        {
            HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        }break;
        
        //��˸
        case LED_BLINK:
        {
            //���㵱ǰʱ��
            ellipsed_time = GetEllipsedTime(led2_blink_start_time);
            
            //�����趨ʱ��
            if(ellipsed_time >= led2_blink_set_time)
            {
                //��LED
                mode_led2 = LED_OFF;
            }
            //δ���趨ʱ��
            else
            {
                ellipsed_time %= (LED2_BLINK_T1_MS + LED2_BLINK_T2_MS);
                if(ellipsed_time <= LED2_BLINK_T1_MS)
                    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
                else
                    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
            }
        }
    }
    
    //����������
    switch(mode_beep)
    {
        //��ʱ�ر�
        case BEEP_DELAY:
        {
            if(GetEllipsedTime(beep_start_time) >= beep_set_time)
            {
                HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4); 
                HAL_GPIO_WritePin(TIM3_CH4_BEEP_GPIO_Port, TIM3_CH4_BEEP_Pin, GPIO_PIN_RESET);
                mode_beep = BEEP_OFF;
            }
        }break;
        
        //�ε��죨�������رգ�
        case BEEP_TICK_ON:
        {
            if(GetEllipsedTime(beep_start_time) >= beep_set_time)
            {
                HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4); 
                HAL_GPIO_WritePin(TIM3_CH4_BEEP_GPIO_Port, TIM3_CH4_BEEP_Pin, GPIO_PIN_RESET);
                
                if((--beep_set_tickes) > 0)
                {
                    beep_start_time = HAL_GetTick();
                    beep_set_time = BEEP_TICK_OFF_MS;
                    mode_beep = BEEP_TICK_OFF;
                }
                else
                {
                    mode_beep = BEEP_OFF;
                }
            }
        }break;
        
        //�ε��죨�������رգ�
        case BEEP_TICK_OFF:
        {
            if(GetEllipsedTime(beep_start_time) >= beep_set_time)
            {
                if((--beep_set_tickes) > 0)
                {
                    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
                    beep_start_time = HAL_GetTick();                 
                    beep_set_time = BEEP_TICK_ON_MS;
                    mode_beep = BEEP_TICK_ON;
                }
                else
                {
                    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4); 
                    HAL_GPIO_WritePin(TIM3_CH4_BEEP_GPIO_Port, TIM3_CH4_BEEP_Pin, GPIO_PIN_RESET);
                    mode_beep = BEEP_OFF;
                }
            }
        }break;
        
        default: ;
    }
}

//Led1����
void Led1TurnOn(void)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    mode_led1 = LED_ON;
}

//Led1�ر�
void Led1TurnOff(void)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    mode_led1 = LED_OFF;
}

//Led1��˸
void Led1Blink(uint16_t ms)
{
    led1_blink_set_time = ms;
    led1_blink_start_time = HAL_GetTick();
    mode_led1 = LED_BLINK;
}

//Led2����
void Led2TurnOn(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
    mode_led2 = LED_ON;
}

//Led2�ر�
void Led2TurnOff(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    mode_led2 = LED_OFF;
}

//Led2��˸
void Led2Blink(uint16_t ms)
{
    led2_blink_set_time = ms;
    led2_blink_start_time = HAL_GetTick();
    mode_led2 = LED_BLINK;
}

//�򿪷�����
void BeepTurnOn(void)
{
    //����PWM
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4); 
    //����״̬
    mode_beep = BEEP_ON;
}

//�رշ�����
void BeepTurnOff(void)
{
    //�ر�PWM
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4); 
    HAL_GPIO_WritePin(TIM3_CH4_BEEP_GPIO_Port, TIM3_CH4_BEEP_Pin, GPIO_PIN_RESET);
    //����״̬
    mode_beep = BEEP_OFF;
}

//��������ʱ
void BeepDelayOff(uint16_t ms)
{
    //����PWM
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4); 
    //����״̬
    beep_set_time = ms;
    beep_start_time = HAL_GetTick();
    mode_beep = BEEP_DELAY;
}

//�������ε�
void BeepDiDi(uint8_t times)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
    beep_set_time = BEEP_TICK_ON_MS;
    beep_set_tickes = times;
    beep_start_time = HAL_GetTick();
    mode_beep = BEEP_TICK_ON;
}



/*
//�ڲ�����������Ϊpwm���ģʽ
void SetPwmMode(void)
{
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 185;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4);
    
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4); 
}

//�ڲ�������ǿ������͵�ƽ
void SetForceMode(void)
{
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4); 
    
    sConfigOC.OCMode = TIM_OCMODE_FORCED_INACTIVE ;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4);
}*/

//�ڲ����������㾭��ʱ��
uint32_t GetEllipsedTime(uint32_t start_time)
{
    uint32_t tick = HAL_GetTick();
    if(tick >= start_time)
    {
        return (tick - start_time);
    }
    else
    {
        return ((0xFFFFFFFF - start_time) + tick);
    }
}


