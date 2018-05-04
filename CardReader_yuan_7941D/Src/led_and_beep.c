
/* Includes ------------------------------------------------------------------*/
#include "led_and_beep.h"
#include "tim.h"

//宏定义区
#define LED1_BLINK_T1_MS 50
#define LED2_BLINK_T1_MS 50
#define LED1_BLINK_T2_MS 500
#define LED2_BLINK_T2_MS 500
#define BEEP_TICK_ON_MS 100
#define BEEP_TICK_OFF_MS 200

//自定义枚举类型
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

//全局变量
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

//内部函数声明区
uint32_t GetEllipsedTime(uint32_t start_time);

//初始化函数
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

//轮询函数
void LedBeepPoll(void)
{
    int ellipsed_time;
        
    //LED1控制
    switch(mode_led1)
    {
        //点亮
        case LED_ON:
        {
            HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        }break;
        
        //熄灭
        case LED_OFF:
        {
            HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        }break;
        
        //闪烁
        case LED_BLINK:
        {
            //计算当前时间
            ellipsed_time = GetEllipsedTime(led1_blink_start_time);
            
            //到达设定时间
            if(ellipsed_time >= led1_blink_set_time)
            {
                //关LED
                mode_led1 = LED_OFF;
            }
            //未到设定时间
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
    
    //LED2控制
    switch(mode_led2)
    {
        //点亮
        case LED_ON:
        {
            HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
        }break;
        
        //熄灭
        case LED_OFF:
        {
            HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        }break;
        
        //闪烁
        case LED_BLINK:
        {
            //计算当前时间
            ellipsed_time = GetEllipsedTime(led2_blink_start_time);
            
            //到达设定时间
            if(ellipsed_time >= led2_blink_set_time)
            {
                //关LED
                mode_led2 = LED_OFF;
            }
            //未到设定时间
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
    
    //蜂鸣器控制
    switch(mode_beep)
    {
        //延时关闭
        case BEEP_DELAY:
        {
            if(GetEllipsedTime(beep_start_time) >= beep_set_time)
            {
                HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4); 
                HAL_GPIO_WritePin(TIM3_CH4_BEEP_GPIO_Port, TIM3_CH4_BEEP_Pin, GPIO_PIN_RESET);
                mode_beep = BEEP_OFF;
            }
        }break;
        
        //滴滴响（定次数关闭）
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
        
        //滴滴响（定次数关闭）
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

//Led1常亮
void Led1TurnOn(void)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    mode_led1 = LED_ON;
}

//Led1关闭
void Led1TurnOff(void)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    mode_led1 = LED_OFF;
}

//Led1闪烁
void Led1Blink(uint16_t ms)
{
    led1_blink_set_time = ms;
    led1_blink_start_time = HAL_GetTick();
    mode_led1 = LED_BLINK;
}

//Led2常亮
void Led2TurnOn(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
    mode_led2 = LED_ON;
}

//Led2关闭
void Led2TurnOff(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    mode_led2 = LED_OFF;
}

//Led2闪烁
void Led2Blink(uint16_t ms)
{
    led2_blink_set_time = ms;
    led2_blink_start_time = HAL_GetTick();
    mode_led2 = LED_BLINK;
}

//打开蜂鸣器
void BeepTurnOn(void)
{
    //开启PWM
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4); 
    //设置状态
    mode_beep = BEEP_ON;
}

//关闭蜂鸣器
void BeepTurnOff(void)
{
    //关闭PWM
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4); 
    HAL_GPIO_WritePin(TIM3_CH4_BEEP_GPIO_Port, TIM3_CH4_BEEP_Pin, GPIO_PIN_RESET);
    //设置状态
    mode_beep = BEEP_OFF;
}

//蜂鸣器定时
void BeepDelayOff(uint16_t ms)
{
    //开启PWM
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4); 
    //设置状态
    beep_set_time = ms;
    beep_start_time = HAL_GetTick();
    mode_beep = BEEP_DELAY;
}

//蜂鸣器滴滴
void BeepDiDi(uint8_t times)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
    beep_set_time = BEEP_TICK_ON_MS;
    beep_set_tickes = times;
    beep_start_time = HAL_GetTick();
    mode_beep = BEEP_TICK_ON;
}



/*
//内部函数：配置为pwm输出模式
void SetPwmMode(void)
{
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 185;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4);
    
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4); 
}

//内部函数：强制输出低电平
void SetForceMode(void)
{
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4); 
    
    sConfigOC.OCMode = TIM_OCMODE_FORCED_INACTIVE ;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4);
}*/

//内部函数：计算经过时间
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


