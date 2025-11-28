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
#include <stdio.h>
#include "speex/speex.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FRAME_SIZE 160 
#define BUFFER_SIZE 50
#define EVER_SIZE 40
#define EVER_SIZE_X2 80
int16_t dma[EVER_SIZE_X2];
int cb_cnt=0;
int rptr,wptr;
signed short pcm[BUFFER_SIZE][FRAME_SIZE];
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// ??PCM??
static uint8_t pcm_test[]= {
  0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 
  0xFF, 0xFF, 0x01, 0x00, 0xFF, 0xFF, 0x02, 0x00, 0xFE, 0xFF, 0x02, 0x00, 0xFC, 0xFF, 0x04, 0x00, 
  0xFB, 0xFF, 0x05, 0x00, 0xFB, 0xFF, 0x06, 0x00, 0xF9, 0xFF, 0x09, 0x00, 0xF3, 0xFF, 0x12, 0x00, 
  0xE3, 0xFF, 0x3A, 0x00, 0x52, 0xFF, 0xFC, 0xFA, 0x59, 0x00, 0xFE, 0x02, 0xF1, 0xFD, 0x2E, 0xFE, 
  0x92, 0x02, 0xD0, 0x03, 0xAE, 0xF9, 0x87, 0xFB, 0x1D, 0xFE, 0xFA, 0xFD, 0xC2, 0x00, 0xAF, 0xFE, 
  0xAD, 0xFC, 0xB3, 0xFC, 0x3E, 0x00, 0xA3, 0x00, 0x23, 0x00, 0x05, 0x05, 0xF6, 0xFC, 0xF0, 0xF7, 
  0x38, 0x02, 0x63, 0x02, 0x0C, 0xFD, 0xAB, 0xF8, 0x06, 0x00, 0x62, 0x04, 0x04, 0xFD, 0xBA, 0xFC, 
  0x3E, 0x01, 0x94, 0xFF, 0x38, 0xFF, 0x83, 0x00, 0x49, 0x00, 0x4A, 0x02, 0xFC, 0xFF, 0x6D, 0x00, 
  0x99, 0x00, 0x50, 0x01, 0xB4, 0x03, 0x7D, 0xFD, 0x4B, 0xFE, 0x09, 0xFF, 0x35, 0x01, 0xA2, 0x02, 
  0xCC, 0xFF, 0xB5, 0xFE, 0x7A, 0xFB, 0xD0, 0xFE, 0xB8, 0xFA, 0xA1, 0x00, 0xC8, 0x03, 0x56, 0xFB, 
  0x76, 0xFD, 0xB6, 0x00, 0x28, 0x06, 0x1E, 0x02, 0x7A, 0xFF, 0xF1, 0x01, 0xB4, 0xFF, 0xB1, 0x02, 
  0xA2, 0x00, 0x22, 0x00, 0x4F, 0x00, 0x5B, 0xFF, 0xC9, 0x01, 0x94, 0x05, 0xDE, 0x00, 0xAC, 0x03, 
  0xC3, 0xFE, 0x10, 0xFC, 0xFF, 0x00, 0x5F, 0xFD, 0x00, 0xFD, 0x92, 0xFD, 0x1F, 0x01, 0x6A, 0x02, 
  0x19, 0xFF, 0x05, 0xFC, 0x2D, 0xFF, 0x4F, 0x02, 0x2C, 0x01, 0x1D, 0x01, 0xE2, 0x02, 0xB2, 0x03, 
  0x04, 0x03, 0x4B, 0x02, 0xAB, 0x03, 0x47, 0xFF, 0x2B, 0xFC, 0xE3, 0xFE, 0x54, 0xFF, 0x3A, 0x00, 
  0xE4, 0xFD, 0xE1, 0xF9, 0x7C, 0xFE, 0x0F, 0xF9, 0xDD, 0xFB, 0xE3, 0xFD, 0x49, 0xFF, 0x7D, 0x09, 
  0xB4, 0x05, 0x7A, 0xFF, 0x9E, 0xFB, 0xDA, 0xFB, 0xA6, 0xFD, 0x2C, 0xFD, 0xC0, 0x05, 0x17, 0x04, 
  0xBC, 0x00, 0x75, 0x07, 0x2A, 0x02, 0x23, 0xFF, 0xA9, 0xFA, 0x68, 0xFE, 0xF7, 0xFE, 0x72, 0xFB, 
  0x2E, 0x04, 0xC4, 0x06, 0xAE, 0x02, 0xBC, 0xFC, 0x73, 0xFF, 0x0F, 0xFD, 0x30, 0xFB, 0x88, 0xFF, 
  0x8D, 0x05, 0x50, 0x05, 0x11, 0x03, 0x55, 0x04, 0x16, 0x04, 0x28, 0xFE, 0x15, 0xFC, 0x33, 0x04, 
  0x58, 0x05, 0x19, 0x03, 0xC0, 0xFE, 0x04, 0x02, 0xB8, 0x02, 0xB0, 0x00, 0x1D, 0xFD, 0x8D, 0xFA, 
  0xF8, 0xFB, 0x4F, 0xFA, 0x5A, 0xFA, 0xA9, 0xFD, 0x16, 0xFD, 0x2A, 0xFD, 0xDC, 0x01, 0xDE, 0xFC, 
  0x59, 0x00, 0x8D, 0x03, 0x42, 0x03, 0x3E, 0x01, 0x33, 0x01, 0x37, 0x00, 0xC0, 0xFD, 0x7D, 0xFE, 
  0x6C, 0xFC, 0x6E, 0x01, 0x58, 0xFD, 0x1B, 0xF8, 0x25, 0xFD, 0x2E, 0xFF, 0x02, 0xFE, 0xEC, 0xFC, 
  0xF0, 0xFE, 0xFB, 0xFE, 0x3E, 0xFC, 0xCB, 0xFD, 0x3F, 0xFB, 0xDB, 0xF9, 0x26, 0xFC, 0xC5, 0xF9,
};

void print_hex(char *data, int size) {
    //printf("数据 (%d 字节): ", size);
    for(int i = 0; i < size; i++) {
        printf("%02X", (uint8_t)data[i]);
    }
    printf("\n");
}

int fputc(int ch, FILE *f) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch,1,20);  // ??USART1,???????
    return ch;
}
int pcm_ready=3; 
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if(GPIO_Pin==GPIO_PIN_0)
  {
    if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0)==0)
    {
			//pcm_ready++;
			HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_2);
			if(pcm_ready==3){
				pcm_ready=1;
				HAL_I2S_Receive_DMA(&hi2s2,(uint16_t *)dma,EVER_SIZE_X2);
			}	
			else{
				pcm_ready=0;
				HAL_I2S_DMAStop(&hi2s2);
			}
    } 
  }
}

void i2s_settle(signed short *orignal_data,signed short *now_data){
	if(orignal_data[0]!=0){
  for(int i=0;i<EVER_SIZE_X2;i+=2){
    now_data[i/2]=orignal_data[i];
  }}
	else{
	for(int i=1;i<EVER_SIZE_X2;i+=2){
    now_data[i/2]=orignal_data[i];
  }}
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
	if(hi2s==&hi2s2){
		//以采样频率的十分之一，串口发送采样值
		if(cb_cnt<4) {
			i2s_settle(dma,pcm[wptr]+cb_cnt*EVER_SIZE);
			cb_cnt++;	
		}
		else {
			wptr = wptr==BUFFER_SIZE-1 ? 0 : wptr + 1;
			cb_cnt=0;	
		}

	}
}
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
	
		SpeexBits g_enc_bits;
		void *g_enc_state;
		int tmp = 0; 
		char enc_char[200];
		int nbBits;
		int QUALITY=7;                  
    speex_bits_init(&g_enc_bits);
		
    g_enc_state = speex_encoder_init(speex_lib_get_mode(SPEEX_MODEID_NB));  
    speex_encoder_ctl(g_enc_state, SPEEX_SET_QUALITY, &QUALITY);
 
		tmp=0;
		speex_encoder_ctl(g_enc_state, SPEEX_SET_VBR, &tmp);
		tmp=7;	
		speex_encoder_ctl(g_enc_state, SPEEX_SET_QUALITY, &tmp);
		tmp=1;
		speex_encoder_ctl(g_enc_state, SPEEX_SET_COMPLEXITY, &tmp);
    tmp=1;
    speex_encoder_ctl(g_enc_state, SPEEX_SET_HIGHPASS, &tmp);
		

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
			if(wptr != rptr&&pcm_ready==1)
			{
				if(cb_cnt>=(FRAME_SIZE/EVER_SIZE))
				{
					speex_bits_reset(&g_enc_bits);
					speex_encode_int(g_enc_state,pcm[rptr], &g_enc_bits);
					nbBits = speex_bits_write(&g_enc_bits, enc_char, 200);
					//printf("字长%d ",nbBytes);	
					print_hex(enc_char,nbBits);
					//HAL_UART_Transmit_DMA(&huart1,(uint8_t *)enc_char,76);
						rptr = (rptr==BUFFER_SIZE-1) ? 0 : rptr+1;
				}
		}
			else if(pcm_ready==0){
				printf("FFFFF");
				printf("\n");pcm_ready=3;
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
