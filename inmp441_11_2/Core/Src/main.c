/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dcmi.h"
#include "dma.h"
#include "i2c.h"
#include "i2s.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdint.h"
#include "stdio.h"
#include <speex/speex_preprocess.h>
#include <speex/speex.h>
#define SAMPLE_RATE 8000
#define FRAME_SIZE 80
#define RAW_FRAME_SIZE FRAME_SIZE*2
SpeexBits bits;
int fputc(int ch, FILE *f) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0x1000);  // 使用USART1，超时时间足够长
    return ch;
}

spx_int16_t pcm_buf[2][FRAME_SIZE];
uint8_t curr_buf_idx = 0; 
volatile uint8_t data_ready = 0;      // 一帧数据就绪标志
SpeexPreprocessState *preprocess_state;

// 4. INMP441 24位→16位PCM转换（提取有效声道）
void INMP441_ConvertTo16bitPCM(spx_int16_t *pcm_out, uint16_t *i2s_in, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        // 步骤1：提取有效声道（假设INMP441接左声道，取偶数索引0,2,4...）
        uint16_t raw_16bit = i2s_in[2 * i];  // 跳过右声道（奇数索引）
        
        // 步骤2：INMP441输出为无符号16位，转换为有符号16位PCM
        // （24位数据截取高16位后，偏移量调整为16位范围）
        pcm_out[i] = (spx_int16_t)(raw_16bit);  // 无符号→有符号
    }
}
void Speex_Init(void) {
		HAL_UART_Transmit(&huart1,"into init",11,0x1000);
    preprocess_state= speex_preprocess_state_init(80, SAMPLE_RATE); 
    if (preprocess_state == NULL) {
        HAL_UART_Transmit(&huart1,"not success",12,0x1000);
    }
		else {
			HAL_UART_Transmit(&huart1,"success",7,0x1000);
		}
    // 配置16位PCM优化参数
    int denoise = 1;                // 开启降噪 
	
    speex_preprocess_ctl(preprocess_state, SPEEX_PREPROCESS_SET_DENOISE, &denoise);
}
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
int16_t dma[80];
spx_int16_t pcm_d[80];
uint32_t val24;
int val32;
int PCM16;
unsigned cb_cnt=0;
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
	if(hi2s==&hi2s2){
		data_ready=1;
		curr_buf_idx = 1-curr_buf_idx;
		cb_cnt++;//回调次数计数
		if(cb_cnt%5==0)
			printf("pcm=%d speex=%d\r\n",dma[0],pcm_buf[curr_buf_idx][0]);
		INMP441_ConvertTo16bitPCM(pcm_buf[curr_buf_idx],dma,80);	
	}
}
//void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
//{
//	if(hi2s==&hi2s2){
//		cb_cnt++;//回调次数计数
//		//将两个32整型合并为一个
//		//dat32 example: 0000fffb 00004f00
//		val24=(dma[0]<<8)+(dma[1]>>8);
//		//将24位有符号整型扩展到32位
//		if(val24 & 0x800000){//negative
//			val32=0xff000000 | val24;
//		}else{//positive
//			val32=val24;
//		}
//		//以采样频率的十分之一，串口发送采样值
//		if(cb_cnt%10==0)
//			printf("%d\r\n",val32);
//	}
//}

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2S2_Init();
  MX_USART1_UART_Init();
  MX_DCMI_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	Speex_Init();
	HAL_I2S_Receive_DMA(&hi2s2,(uint16_t*)dma,80);	
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		{
       if (data_ready) {
        // 1. 取“已完成接收”的缓冲区（不是当前正在接收的）
        uint8_t process_idx = 1 - curr_buf_idx;
					printf("pcm=\r\n");
				   for (int i = 0; i < 80; i++){
						printf("%d ",pcm_buf[curr_buf_idx][i]);
						 if(i%5==0){
							printf("\r\n");
						 }
					}
        // 2. 处理该缓冲区（此时 DMA 正在写另一个缓冲区，不会覆盖当前数据）
				int16_t frame1[80];
				memcpy(frame1,pcm_buf[curr_buf_idx],40*sizeof(int16_t *));
        speex_preprocess_run(preprocess_state,frame1);
        // 执行编码...
				printf("speex=\r\n");
				for (int i = 0; i < 80; i++){
					printf("%d",frame1	[i]);
					if(i%5==0){
						printf("\r\n");
					 }
					}
        // 3. 清除就绪标志，等待下一帧
				data_ready = 0;
				//HAL_I2S_Receive_DMA(&hi2s2,(uint16_t*)dma,160);	
			}   
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
