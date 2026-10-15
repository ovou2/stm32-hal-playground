/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t Rxbuff[3] = {0};   /* 接收缓冲区：逐字节拼命令用，3 格正好装下 "OFF" */
uint8_t count   = 0;       /* 接收计数：记「本次命令已收到第几个字节」——中断分批进来，跨批次的进度必须用全局变量记 */
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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  /* 挂上接收：length=1，所以每来 1 个字节就进一次回调（单字节模式）。
     第一次写入位置是 Rxbuff[0]，之后在回调里会挂到 &Rxbuff[count] 往后继续拼。 */
  HAL_UART_Receive_IT(&huart2, Rxbuff, 1);

  printf("ON=led open OFF=led down\r\n");   /* 开机提示：告诉 PC 支持哪些命令 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
  * @brief  接收完成回调 —— 每收到 1 个字节就会被调用一次（挂接收时 length=1）
  * @param  huart 触发这次回调的串口句柄
  * @note   函数名不能改：它是 HAL 里的 __weak 空函数，同名重写才会被调用
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)        /* 多个串口共用一个回调，必须判断是谁触发的 */
  {
    count++;                            /* 数「这是本次命令的第几个字节」；只有全局变量能跨回调记住它 */

    /* 长度 + 内容双条件：先数够 2 个，再比对是不是 "ON"。
       只比内容不比长度的话，收到 "ONx" 这种 3 字节数据也会误判点灯。 */
    if (count == 2 && strncmp((char *)Rxbuff, "ON", 2) == 0)
    {
      printf("LED ON\r\n");
      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);    /* 点亮 LED */
      count = 0;                              /* 一条命令处理完：计数归零 */
      memset(Rxbuff, 0, sizeof(Rxbuff));      /* 缓冲区也清掉，防止旧字符残留影响下一条命令 */
    }
    else if (count == 3 && strncmp((char *)Rxbuff, "OFF", 3) == 0)
    {
      printf("LED OFF\r\n");
      HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);  /* 熄灭 LED */
      count = 0;
      memset(Rxbuff, 0, sizeof(Rxbuff));
    }

    /* ★ 越界保护：Rxbuff 只有 3 格。如果收满 3 个还没拼中任何命令
       （比如发了 "ABC" 这种杂鱼数据），count 已到 3，
       再往 &Rxbuff[3] 写就越界、会破坏相邻内存 —— 必须强制复位重来。 */
    if (count >= 3)
    {
      count = 0;
      memset(Rxbuff, 0, sizeof(Rxbuff));
    }

    /* ★ 最关键的一行：接收中断是「一次性」的，每收 1 个 HAL 就自动把它关掉。
       必须重新挂上；而且注意写入位置是 &Rxbuff[count] ——
       下一个字节落到正确的格子，命令才能一个个字符拼起来。 */
    HAL_UART_Receive_IT(&huart2, &Rxbuff[count], 1);
  }
}

/**
  * @brief  printf 重定向 —— printf 每输出一个字符，都会回头调用这个函数
  * @note   需在 Keil 勾选 Options for Target → Target → Use MicroLIB
  */
int fputc(int ch, FILE *f)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);   /* 阻塞发送这 1 个字节 */
  return ch;
}
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
