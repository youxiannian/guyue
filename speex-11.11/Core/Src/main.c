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
#include "speex.h"
#include <stdio.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "speex.h"

// 配置参数
#define SAMPLE_RATE 8000           // 8kHz 采样率
#define FRAME_SIZE 80              // 10ms 帧大小
#define MAX_ENCODED_BYTES 50       // 编码后最大字节数
int QUALITY=6;                  // 编码质量 (0-10);

// 全局状态
static SpeexBits g_enc_bits;
static SpeexBits g_dec_bits;
static void *g_enc_state = NULL;
static void *g_dec_state = NULL;
static int g_initialized = 0;

// 测试PCM数据
short pcm_array[80] = {
    0xF0FE, 0xFBFE, 0x3EFC, 0xCBFD, 0x3FFB, 0xDBF9, 0x26FC, 0xC5F9,
    0x30FF, 0x0F02, 0xC6FE, 0x81FD, 0x68FA, 0x06F8, 0xC8FB, 0x0602,
    0xA804, 0xF002, 0xF0FF, 0x9A00, 0x92FD, 0x7BFC, 0x26FF, 0x6901,
    0x0302, 0x6C04, 0x2F07, 0x0905, 0x3702, 0xE900, 0xDEFF, 0x7C01,
    0x2703, 0x5702, 0xC901, 0xFA03, 0xB102, 0x4DFF, 0xF000, 0x91FF,
    0xA9FF, 0x1A04, 0xF205, 0xFF05, 0x2901, 0xEB03, 0xC108, 0x2B05,
    0x4B00, 0xCCFE, 0x2902, 0x3503, 0xECFE, 0x3201, 0x0105, 0xC8FF,
    0xE1F8, 0x19FB, 0xD2FE, 0x02FE, 0x2AF8, 0x8BF8, 0xCDFE, 0xDEFD,
    0x1FFD, 0xD6FC, 0x90FC, 0x8FFB, 0xDDF9, 0x8BF9, 0x70FB, 0xFCFD,
    0x12FD, 0xDCFE, 0x7202, 0xAA00, 0xA9FE, 0x74FD, 0x2D00, 0x2501
};

/**
 * @brief 十六进制打印数据
 * @param data 数据缓冲区
 * @param size 数据大小
 */
void print_hex(const char *data, int size) {
    printf("数据 (%d 字节): ", size);
    for(int i = 0; i < size; i++) {
        printf("%02X ", (unsigned char)data[i]);
    }
    printf("\n");
}

/**
 * @brief 打印PCM数据
 * @param pcm PCM数据缓冲区
 * @param size 数据大小
 */
void print_pcm(const short *pcm, int size) {
    printf("PCM数据 (%d 样本):\n", size);
    for(int i = 0; i < size; i++) {
        printf("%6d ", pcm[i]);
        if((i + 1) % 10 == 0) printf("\n");
    }
    printf("\n");
}

/**
 * @brief 初始化Speex编解码器
 * @return 成功返回0，失败返回-1
 */
int speex_codec_init(void) {
    // 初始化编码器
    speex_bits_init(&g_enc_bits);
    g_enc_state = speex_encoder_init(&speex_nb_mode);
    if (g_enc_state == NULL) {
        printf("❌ 编码器初始化失败\n");
        speex_bits_destroy(&g_enc_bits);
        return -1;
    }
    
    // 设置编码参数
    speex_encoder_ctl(g_enc_state, SPEEX_SET_QUALITY, &QUALITY);
    int vbr = 0;  // 关闭VBR，使用固定比特率
    speex_encoder_ctl(g_enc_state, SPEEX_SET_VBR, &vbr);
    
    // 初始化解码器
    speex_bits_init(&g_dec_bits);
    g_dec_state = speex_decoder_init(&speex_nb_mode);
    if (g_dec_state == NULL) {
        printf("❌ 解码器初始化失败\n");
        speex_bits_destroy(&g_enc_bits);
        speex_encoder_destroy(g_enc_state);
        return -1;
    }
    
    // 设置解码器增强
    int enh = 1;
    speex_decoder_ctl(g_dec_state, SPEEX_SET_ENH, &enh);
    
    g_initialized = 1;
    printf("✅ Speex编解码器初始化成功 (质量%d)\n", QUALITY);
    
    // 显示配置信息
    int frame_size;
    speex_encoder_ctl(g_enc_state, SPEEX_GET_FRAME_SIZE, &frame_size);
    printf("配置: 帧大小=%d样本, 采样率=%dHz\n", frame_size, SAMPLE_RATE);
    
    return 0;
}

/**
 * @brief 编码PCM数据
 * @param pcm_input 输入的PCM数据
 * @param output_buffer 输出编码数据缓冲区
 * @return 编码数据字节数，失败返回-1
 */
int speex_encode_frame(const short *pcm_input, char *output_buffer) {

    
    // 编码过程
    speex_bits_reset(&g_enc_bits);
    speex_encode_int(g_enc_state, (spx_int16_t *)pcm_input, &g_enc_bits);
    int nbBytes = speex_bits_write(&g_enc_bits, output_buffer, MAX_ENCODED_BYTES);
    
    if (nbBytes <= 0) {
        printf("❌ 编码失败\n");
        return -1;
    }
    
    return nbBytes;
}

/**
 * @brief 解码Speex数据
 * @param encoded_data 输入的编码数据
 * @param encoded_size 编码数据大小
 * @param pcm_output 输出PCM数据缓冲区
 * @return 成功返回0，失败返回-1
 */
int speex_decode_frame(const char *encoded_data, int encoded_size, short *pcm_output) {
    
    // 解码过程
    speex_bits_reset(&g_dec_bits);
    speex_bits_read_from(&g_dec_bits, encoded_data, encoded_size);
    int ret = speex_decode_int(g_dec_state, &g_dec_bits, (spx_int16_t *)pcm_output);
    
    if (ret != 0) {
        printf("❌ 解码失败，错误码: %d\n", ret);
        return -1;
    }
    
    return 0;
}

/**
 * @brief 比较两个PCM帧的差异
 * @param orig 原始PCM数据
 * @param decoded 解码后的PCM数据
 * @param size 数据大小
 */
void compare_pcm_frames(const short *orig, const short *decoded, int size) {
    printf("=== PCM数据对比 ===\n");
    
    int max_diff = 0;
    int diff_count = 0;
    float total_diff = 0;
    
    for(int i = 0; i < size; i++) {
        int diff = abs(orig[i] - decoded[i]);
        if(diff > 0) {
            diff_count++;
            total_diff += diff;
            if(diff > max_diff) max_diff = diff;
        }
        
        // 打印前10个样本的对比
        if(i < 10) {
            printf("样本[%2d]: 原始=%6d, 解码=%6d, 差异=%6d\n", 
                   i, orig[i], decoded[i], diff);
        }
    }
    
    printf("\n对比统计:\n");
    printf("不同样本数: %d/%d (%.1f%%)\n", 
           diff_count, size, (diff_count * 100.0) / size);
    printf("最大差异: %d\n", max_diff);
    printf("平均差异: %.2f\n", total_diff / size);
    printf("压缩比: %.2f:1 (原始:%d字节 -> 编码后:约%d字节)\n", 
           (size * 2.0) / 25.0, size * 2, 25); // 估算
}

/**
 * @brief 清理资源
 */
void speex_codec_cleanup(void) {
    if (g_enc_state != NULL) {
        speex_encoder_destroy(g_enc_state);
        g_enc_state = NULL;
    }
    if (g_dec_state != NULL) {
        speex_decoder_destroy(g_dec_state);
        g_dec_state = NULL;
    }
    speex_bits_destroy(&g_enc_bits);
    speex_bits_destroy(&g_dec_bits);
    g_initialized = 0;
    printf("✅ Speex编解码器资源已清理\n");
}

/**
 * @brief 完整的编码解码测试
 */
void complete_speex_test(void) {
    printf("\n");
    printf("========================================\n");
    printf("        Speex 编解码完整测试\n");
    printf("========================================\n");
    
    // 1. 初始化
    printf("\n1. 初始化编解码器...\n");
    if (speex_codec_init() != 0) {
        printf("初始化失败，退出测试\n");
        return;
    }
    
    // 2. 显示原始PCM数据
    printf("\n2. 原始PCM数据:\n");
    print_pcm(pcm_array, FRAME_SIZE);
    
    // 3. 编码
    printf("\n3. 开始编码...\n");
    char encoded_data[MAX_ENCODED_BYTES];
    int encoded_size = speex_encode_frame(pcm_array, encoded_data);
    
    if (encoded_size <= 0) {
        printf("编码失败，退出测试\n");
        speex_codec_cleanup();
        return;
    }
    
    printf("✅ 编码成功\n");
    print_hex(encoded_data, encoded_size);
    
    // 4. 解码
    printf("\n4. 开始解码...\n");
    short decoded_pcm[FRAME_SIZE];
    if (speex_decode_frame(encoded_data, encoded_size, decoded_pcm) != 0) {
        printf("解码失败，退出测试\n");
        speex_codec_cleanup();
        return;
    }
    
    printf("✅ 解码成功\n");
    print_pcm(decoded_pcm, FRAME_SIZE);
    
    // 5. 对比分析
    printf("\n5. 编码解码质量分析:\n");
    compare_pcm_frames(pcm_array, decoded_pcm, FRAME_SIZE);
    
    // 6. 清理资源
    printf("\n6. 清理资源...\n");
    speex_codec_cleanup();
    
    printf("\n========================================\n");
    printf("             测试完成\n");
    printf("========================================\n");
}

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
int fputc(int ch, FILE *f) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0x1000);  // 使用USART1，超时时间足够长
    return ch;
}
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
	printf("Speex 编码解码演示程序\n");
  printf("采样率: %d Hz, 帧大小: %d 样本, 质量: %d\n\n", 
  SAMPLE_RATE, FRAME_SIZE, QUALITY);

  complete_speex_test();
  //HAL_I2S_Receive_DMA(&hi2s2,(uint16_t*)dma,80);	

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		
		
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
