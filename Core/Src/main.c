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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include<stdio.h>
#include <string.h>

#include "oled.h"
#include "aht20.h"
#include "w25q64.h"
#include "config.h"
#include "alarm.h"

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
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

static volatile uint8_t button_pressed = 0;

static char tx_buffer[64];

static char oled_temp[16];
static char oled_humi[16];
static char oled_state[16];

static float humidity = 0.0f;
static float temperature = 0.0f;

static int32_t temperature_x10 = 0;
static uint32_t humidity_x10 = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
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
  MX_TIM2_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */

  HAL_Delay(100);

  HAL_StatusTypeDef aht20_init_status;

  aht20_init_status = AHT20_Init(&hi2c1);


  int len = snprintf(
      tx_buffer,
      sizeof(tx_buffer),
      "AHT20 Init: %s\r\n",
      (aht20_init_status == HAL_OK) ? "OK" : "ERROR"
  );

  HAL_StatusTypeDef oled_init_status;

  oled_init_status = OLED_Init(&hi2c1);

  if (oled_init_status == HAL_OK)
  {
	    OLED_Clear();
  }

  HAL_UART_Transmit(
      &huart2,
      (uint8_t *)tx_buffer,
      len,
      100
  );


  int oled_len = snprintf(
      tx_buffer,
      sizeof(tx_buffer),
      "OLED Init: %s\r\n",
      (oled_init_status == HAL_OK) ? "OK" : "ERROR"
  );

  HAL_UART_Transmit(
      &huart2,
      (uint8_t *)tx_buffer,
      oled_len,
      100
  );


  uint8_t flash_manufacturer;
  uint8_t flash_memory_type;
  uint8_t flash_capacity;

  HAL_StatusTypeDef flash_status;

  flash_status = W25Q64_Init(&hspi1);

  if (flash_status == HAL_OK)
  {
      flash_status = W25Q64_ReadJEDEC_ID(&flash_manufacturer,
                                        &flash_memory_type,
                                        &flash_capacity);
  }

  if (flash_status == HAL_OK)
  {
      int flash_len = snprintf(
          tx_buffer,
          sizeof(tx_buffer),
          "W25Q64 JEDEC ID: %02X %02X %02X\r\n",
          flash_manufacturer,
          flash_memory_type,
          flash_capacity
      );

      HAL_UART_Transmit(
          &huart2,
          (uint8_t *)tx_buffer,
          flash_len,
          100
      );
  }
  else
  {
      int flash_len = snprintf(
          tx_buffer,
          sizeof(tx_buffer),
          "W25Q64 ERROR\r\n"
      );

      HAL_UART_Transmit(
          &huart2,
          (uint8_t *)tx_buffer,
          flash_len,
          100
      );
  }



  AlarmConfig read_config;


  if(Config_Load(&read_config) == HAL_OK &&
		   Config_IsValid(&read_config))
  {
      int config_len = snprintf(
          tx_buffer,
          sizeof(tx_buffer),
          "Load Config: T_ON=%d T_OFF=%d H_ON=%d H_OFF=%d\r\n",
          read_config.temp_on_x10,
          read_config.temp_off_x10,
          read_config.humi_on_x10,
          read_config.humi_off_x10
      );


      HAL_UART_Transmit(
          &huart2,
          (uint8_t *)tx_buffer,
          config_len,
          100
      );

      snprintf(
          tx_buffer,
          sizeof(tx_buffer),
          "Config source: FLASH\r\n"
      );

      HAL_UART_Transmit(
          &huart2,
          (uint8_t *)tx_buffer,
          strlen(tx_buffer),
          100
      );


  }
  else
  {
      /* Flash读取失败或配置非法 */
      Config_SetDefault(&read_config);

      Config_Save(&read_config);


      snprintf(
          tx_buffer,
          sizeof(tx_buffer),
          "Config invalid, defaults restored\r\n"
      );

      HAL_UART_Transmit(
          &huart2,
          (uint8_t *)tx_buffer,
          strlen(tx_buffer),
          100
      );
  }


  Alarm_SetThresholds(
      read_config.temp_on_x10,
      read_config.temp_off_x10,
      read_config.humi_on_x10,
      read_config.humi_off_x10
  );



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  if (button_pressed == 1)
	  {
	      button_pressed = 0;

	      Alarm_Enable_Toggle();
	  }


	  if (AHT20_Read(&temperature, &humidity) == HAL_OK)
	  {

		  temperature_x10 = (int32_t)(temperature * 10.0f);
		  humidity_x10 = (uint32_t)(humidity * 10.0f);

		  Alarm_Update(
		      temperature_x10,
		      humidity_x10
		  );


		  Alarm_Buzzer_Update();
		  /* ---------- UART 输出 ---------- */

		  int aht_len = snprintf(
		      tx_buffer,
		      sizeof(tx_buffer),
		      "Temperature: %ld.%ld C, Humidity: %lu.%lu %%, State: %s\r\n",
		      (long)(temperature_x10 / 10),
		      (long)(temperature_x10 % 10),
		      (unsigned long)(humidity_x10 / 10),
		      (unsigned long)(humidity_x10 % 10),
		      (Alarm_GetState() == ALARM_ACTIVE) ? "ALARM" : "NORMAL"
		  );

		  HAL_UART_Transmit(
			  &huart2,
			  (uint8_t *)tx_buffer,
			  aht_len,
			  100
		  );


		  /* ---------- OLED 实时温度 ---------- */

		  snprintf(
			  oled_temp,
			  sizeof(oled_temp),
			  "%ld.%ld",
			  (long)(temperature_x10 / 10),
			  (long)(temperature_x10 % 10)
		  );

		  snprintf(
			  oled_humi,
			  sizeof(oled_humi),
			  "%lu.%lu",
			  (unsigned long)(humidity_x10 / 10),
			  (unsigned long)(humidity_x10 % 10)
		  );

		  if (Alarm_GetState() == ALARM_ACTIVE)
		  {
		      snprintf(
		          oled_state,
		          sizeof(oled_state),
		          "ALARM"
		      );
		  }
		  else
		  {
		      snprintf(
		          oled_state,
		          sizeof(oled_state),
		          "NORMAL"
		      );
		  }

		  if (oled_init_status == HAL_OK)
		  {
			  /* 第一行：温度 */
			  OLED_ClearPage(0);
			  OLED_ShowString(0, 0, oled_temp);

			  /* 第二行：湿度 */
			  OLED_ClearPage(1);
			  OLED_ShowString(0, 1, oled_humi);

			  OLED_ClearPage(2);
			  OLED_ShowString(0, 2, oled_state);

			  OLED_ClearPage(3);

			  if (Alarm_IsEnabled())
			  {
			      OLED_ShowString(0, 3, "ALARM ON");
			  }
			  else
			  {
			      OLED_ShowString(0, 3, "ALARM OFF");
			  }
		  }



	  }

	  HAL_Delay(1000);
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, BUZZER_Pin|FLASH_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : BUZZER_Pin FLASH_CS_Pin */
  GPIO_InitStruct.Pin = BUZZER_Pin|FLASH_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last_button_tick = 0;

    if (GPIO_Pin == GPIO_PIN_13)
    {
        uint32_t now = HAL_GetTick();

        if (now - last_button_tick >= 50)
        {
            last_button_tick = now;
            button_pressed = 1;
        }
    }
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
