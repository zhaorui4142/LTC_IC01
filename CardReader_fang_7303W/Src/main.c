/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_hal.h"
#include "dma.h"
#include "iwdg.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "mb.h"
#include "dip.h"
#include "7303w.h"
#include "led_and_beep.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
/* 私有宏定义 ----------------------------------------------------------------*/
#define REG_INPUT_START   30000
#define REG_INPUT_NREGS   4
#define REG_HOLDING_START 40000
#define REG_HOLDING_NREGS 100

/* 私有变量 ------------------------------------------------------------------*/
static uint16_t   usRegInputStart = REG_INPUT_START;
static uint16_t   usRegInputBuf[REG_INPUT_NREGS];
static uint16_t   usRegHoldingStart = REG_HOLDING_START;
static uint16_t   usRegHoldingBuf[REG_HOLDING_NREGS];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

int main(void)
{

    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration----------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_TIM3_Init();
    //MX_USART1_UART_Init();
    MX_USART3_UART_Init();
    MX_TIM4_Init();
    MX_TIM2_Init();
    MX_IWDG_Init();

    /* USER CODE BEGIN 2 */
  

    
    //modbus初始化
    uint8_t SlaveAddr = DIP_GetAddress();   //从站地址
    if(SlaveAddr ==0 || SlaveAddr > 247)
    {
        Error_Handler();
    }
    uint32_t badurate = DIP_GetBadurate();  //波特率
    eMBParity parity = MB_PAR_NONE;         //校验位
    if(DIP_GetParity() == 1)    parity = MB_PAR_ODD;
    if(DIP_GetParity() == 2)    parity = MB_PAR_EVEN;
       
    if(DIP_GetDataBitsLen() == 7)           //数据位
    {
        if(parity != MB_PAR_NONE)   huart1.Init.WordLength = (uint32_t)UART_WORDLENGTH_8B;
        else                        Error_Handler();
    }
    else
    {
        if(parity != MB_PAR_NONE)   huart1.Init.WordLength = (uint32_t)UART_WORDLENGTH_9B;
        else                        huart1.Init.WordLength = (uint32_t)UART_WORDLENGTH_8B;
    }
    
    if(DIP_GetStopBitsLen() == 1)           //停止位
    {
        huart1.Init.StopBits = UART_STOPBITS_1;
    }
    else
    {
        huart1.Init.StopBits = UART_STOPBITS_2;
    }
    
    eMBInit(MB_RTU, SlaveAddr, 3, badurate, parity);
    //eMBInit(MB_RTU, 0x01, 3, 9600, MB_PAR_NONE);
    //eMBErrorCode eMBSetSlaveID( UCHAR ucSlaveID, BOOL xIsRunning,UCHAR const *pucAdditional, USHORT usAdditionalLen );
    eMBEnable();
    
    //读卡器初始化
    RfidUartInit(&huart3, usRegHoldingBuf);//
    
    //灯和蜂鸣器初始化
    LedBeepInit();
    
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    const char str[40] = "zhaorui 18761309466 lygcosco 2017-09-21";
    for(int i=0; i<40; i++)
    {
        (usRegHoldingBuf+50)[i] = str[i];
    }
        
    BeepTurnOn();
    Led1TurnOn();
    HAL_Delay(200);
    Led1TurnOff();
    Led2TurnOn();
    HAL_Delay(200);
    Led2TurnOff();
    BeepTurnOff();
    
    //启动看门狗
    HAL_IWDG_Start(&hiwdg);

    while (1)
    {
        //modbus状态机
        (void)eMBPoll();
      
        //RFID读卡器状态机
        RfidPoll();
      
        //led和蜂鸣器控制
        LedBeepPoll();
      
        //喂狗
        HAL_IWDG_Refresh(&hiwdg);
      
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */

    }
    /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* USER CODE BEGIN 4 */


//定时器中断回调函数-----------------------------------------------
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
/* USER CODE BEGIN Callback 0 */

/* USER CODE END Callback 0 */

/* USER CODE BEGIN Callback 1 */
  //定时器4update中断，处理freemodbus的porttimer中的超时
  if (htim->Instance == TIM4)
  {
      //Freemodbus中断服务函数
      prvvTIMERExpiredISR();
  }      
/* USER CODE END Callback 1 */
}
//-----------------------------------------------------------------

//串口接收完成回调函数---------------------------------------------
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        //Freemodbus中断服务函数
        prvvUARTRxISR();
    }
    
    if(huart->Instance == USART3)
    {
        //Rfid接收完成中断服务函数
        RfidUartRxCpltISR();
    }
}
//------------------------------------------------------------------

//串口发送完成回调函数----------------------------------------------
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        //Freemodbus中断服务函数
        prvvUARTTxReadyISR();//发送完成中断
    }
    if(huart->Instance == USART3)
    {
        //读卡器发送完成中断服务函数
        RfidUartTxCpltISR();
    }
    
}

//DMA发送完成会回调函数-----------------------------------------------

//FreeModbus输入寄存器回调函数，pucRegBuffer数据指针, usAddress数据地址, usNRegs寄存器数量 
eMBErrorCode eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

	usRegInputBuf[0] = 0x1111;
	usRegInputBuf[1] = 0x2222;
	usRegInputBuf[2] = 0x3333;
	usRegInputBuf[3] = 0x4444;
	
    if((usAddress >= REG_INPUT_START) && (usAddress+usNRegs <= REG_INPUT_START+REG_INPUT_NREGS))
    {
        iRegIndex=(int)(usAddress-usRegInputStart);
        while( usNRegs > 0 )
        {
            *pucRegBuffer++ = (UCHAR)(usRegInputBuf[iRegIndex]>>8);
            *pucRegBuffer++ = (UCHAR)(usRegInputBuf[iRegIndex]&0xFF);
            iRegIndex++;
            usNRegs--;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    
    return eStatus;
}

//FreeModbus保持寄存器回调函数，eMode模式(MB_REG_READ读保持寄存器,MB_REG_WRITE写保持寄存器)
eMBErrorCode eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;
	
    if((usAddress >= usRegHoldingStart) && (usAddress+usNRegs <= usRegHoldingStart+REG_HOLDING_NREGS))
    {
        iRegIndex = (int)(usAddress-usRegHoldingStart-1);
        if(eMode == MB_REG_READ)
        {
            while( usNRegs > 0 )
            {
                *pucRegBuffer++ = (UCHAR)(usRegHoldingBuf[iRegIndex]>>8);
                *pucRegBuffer++ = (UCHAR)(usRegHoldingBuf[iRegIndex]&0xFF);
                iRegIndex++;
                usNRegs--;
            }
        }
        if(eMode == MB_REG_WRITE)
        {
            while( usNRegs > 0 )
            {
                usRegHoldingBuf[iRegIndex] = (((uint16_t)pucRegBuffer[0])<<8) | ((uint16_t)pucRegBuffer[1]);
                pucRegBuffer+=2;
                iRegIndex++;
                usNRegs--;
            }
        }
        
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    
    return eStatus;
}


eMBErrorCode eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils,eMBRegisterMode eMode )
{
  return MB_ENOREG;
}

eMBErrorCode eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
  return MB_ENOREG;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
