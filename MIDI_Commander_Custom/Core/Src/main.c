/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "ssd1306_tests.h"
#include <stdbool.h>
#include <stdio.h>
#include "usbd_midi_if.h"
#include "midi_defines.h"
#include "midi_cmds.h"
#include "switch_router.h"
#include "display.h"

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
I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_tx;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_tx;

/* USER CODE BEGIN PV */
uint8_t f_sys_config_complete = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#define EXPRESSION_PROBE_MODE 0
#define BATTERY_PROBE_MODE 0

typedef struct {
	uint32_t channel;
	const char *label;
} adc_probe_chan_t;

static const adc_probe_chan_t adc_probe_channels[] = {
	{0, "CH0 PA0"},
	{1, "CH1 PA1"},
	{2, "CH2 PA2"},
	{3, "CH3 PA3"},
	{4, "CH4 PA4"},
	{5, "CH5 PA5"},
	{6, "CH6 PA6"},
	{7, "CH7 PA7"},
	{8, "CH8 PB0"},
	{9, "CH9 PB1"},
	{10, "CH10 PC0"},
	{11, "CH11 PC1"},
	{12, "CH12 PC2"},
	{13, "CH13 PC3"},
	{14, "CH14 PC4"},
	{15, "CH15 PC5"},
};

static void expression_probe_gpio_init(void){
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;

	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|
	                      GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|
	                      GPIO_PIN_4|GPIO_PIN_5;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

static void adc1_init(void){
	// Enable ADC1 clock
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
	// ADC prescaler: PCLK2 / 6 to stay within ADC max clock
	RCC->CFGR &= ~RCC_CFGR_ADCPRE;
	RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6;

	// Power on ADC
	ADC1->CR2 = 0;
	ADC1->CR1 = 0;
	ADC1->CR2 |= ADC_CR2_ADON;
	for (volatile int i = 0; i < 10000; i++) { __NOP(); }

	// Reset calibration
	ADC1->CR2 |= ADC_CR2_RSTCAL;
	while (ADC1->CR2 & ADC_CR2_RSTCAL) { }

	// Calibrate
	ADC1->CR2 |= ADC_CR2_CAL;
	while (ADC1->CR2 & ADC_CR2_CAL) { }

	// Re-enable after calibration
	ADC1->CR2 |= ADC_CR2_ADON;
}

static void adc_set_sample_time(uint32_t channel){
	// Set sample time to 239.5 cycles for stability
	if (channel <= 9){
		uint32_t shift = (channel * 3);
		ADC1->SMPR2 &= ~(0x7 << shift);
		ADC1->SMPR2 |= (0x7 << shift);
	} else {
		uint32_t shift = ((channel - 10) * 3);
		ADC1->SMPR1 &= ~(0x7 << shift);
		ADC1->SMPR1 |= (0x7 << shift);
	}
}

static uint16_t adc_read_channel(uint32_t channel){
	adc_set_sample_time(channel);

	ADC1->SQR1 = 0; // 1 conversion
	ADC1->SQR3 = channel & 0x1F;

	ADC1->SR = 0;
	ADC1->CR2 |= ADC_CR2_ADON;
	ADC1->CR2 |= ADC_CR2_EXTTRIG;
	ADC1->CR2 |= ADC_CR2_SWSTART;

	uint32_t timeout = 200000;
	while (!(ADC1->SR & ADC_SR_EOC)) {
		if (--timeout == 0){
			return 0;
		}
	}
	return (uint16_t)(ADC1->DR & 0xFFFF);
}

static void expression_probe_loop(void){
	static uint32_t last_tick = 0;
	static size_t idx = 0;
	static uint8_t prev_sw = 1;

	uint8_t sw_now = HAL_GPIO_ReadPin(SW_A_GPIO_Port, SW_A_Pin);
	if (prev_sw && !sw_now){
		idx = (idx + 1) % (sizeof(adc_probe_channels) / sizeof(adc_probe_channels[0]));
	}
	prev_sw = sw_now;

	if (HAL_GetTick() - last_tick < 120){
		return;
	}
	last_tick = HAL_GetTick();

	const adc_probe_chan_t *ch = &adc_probe_channels[idx];
	uint16_t value = adc_read_channel(ch->channel);

	char line1[20];
	char line2[20];
	char line3[20];
	snprintf(line1, sizeof(line1), "EXP PROBE");
	snprintf(line2, sizeof(line2), "%s v%4u", ch->label, value);
	snprintf(line3, sizeof(line3), "A=NEXT CH");

	ssd1306_Fill(Black);
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString(line1, Font_7x10, White);
	ssd1306_SetCursor(0, 20);
	ssd1306_WriteString(line2, Font_7x10, White);
	ssd1306_SetCursor(0, 40);
	ssd1306_WriteString(line3, Font_7x10, White);
	ssd1306_UpdateScreen();
}

static void battery_probe_loop(void){
	static uint32_t last_tick = 0;
	static size_t idx = 0;
	static uint8_t prev_sw = 1;

	uint8_t sw_now = HAL_GPIO_ReadPin(SW_A_GPIO_Port, SW_A_Pin);
	if (prev_sw && !sw_now){
		idx = (idx + 1) % (sizeof(adc_probe_channels) / sizeof(adc_probe_channels[0]));
	}
	prev_sw = sw_now;

	if (HAL_GetTick() - last_tick < 120){
		return;
	}
	last_tick = HAL_GetTick();

	const adc_probe_chan_t *ch = &adc_probe_channels[idx];
	uint16_t value = adc_read_channel(ch->channel);

	char line1[20];
	char line2[20];
	char line3[20];
	snprintf(line1, sizeof(line1), "BAT PROBE");
	snprintf(line2, sizeof(line2), "%s v%4u", ch->label, value);
	snprintf(line3, sizeof(line3), "A=NEXT CH");

	ssd1306_Fill(Black);
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString(line1, Font_7x10, White);
	ssd1306_SetCursor(0, 20);
	ssd1306_WriteString(line2, Font_7x10, White);
	ssd1306_SetCursor(0, 40);
	ssd1306_WriteString(line3, Font_7x10, White);
	ssd1306_UpdateScreen();
}

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
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  // Reset the USB interface in case it's still plugged in.
  HAL_GPIO_WritePin(USB_ID_GPIO_Port, USB_ID_Pin, GPIO_PIN_RESET);

  display_init();

  if (EXPRESSION_PROBE_MODE || BATTERY_PROBE_MODE){
	  expression_probe_gpio_init();
	  adc1_init();
  }

  // Check we've got a 256kB device, in case Melo switch to a smaller device at some point
  uint16_t flash_size = (*(uint16_t*)FLASHSIZE_BASE);
  uint16_t min_size = 256;
  if(flash_size < min_size) {
	  char msg[25];
	  sprintf(msg, "Mem %3dkb < %3dkb", flash_size, min_size);
	  Error(msg);
  }

  // Skip config name to avoid showing CC25 Momentary on boot

  sw_led_init();

  HAL_Delay(1000);
  HAL_GPIO_WritePin(USB_ID_GPIO_Port, USB_ID_Pin, GPIO_PIN_SET);

  HAL_Delay(200);
  f_sys_config_complete = 1; // Don't scan switch changes until everything is init'd
  display_setBankName(0);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  display_tick();
	  if (EXPRESSION_PROBE_MODE){
		  expression_probe_loop();
	  } else if (BATTERY_PROBE_MODE){
		  battery_probe_loop();
	  } else {
		  handle_switches();
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
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
  hi2c1.Init.ClockSpeed = 400000;
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
  huart2.Init.BaudRate = 31250;
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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, LED_C_Pin|LED_B_Pin|USB_ID_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LED_D_Pin|LED_2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_E_Pin|LED_5_Pin|LED_4_Pin|LED_3_Pin
                          |LED_1_Pin|LED_A_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED_C_Pin LED_B_Pin USB_ID_Pin */
  GPIO_InitStruct.Pin = LED_C_Pin|LED_B_Pin|USB_ID_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : SW_B_Pin */
  GPIO_InitStruct.Pin = SW_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SW_B_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SW_C_Pin SW_D_Pin SW_E_Pin SW_2_Pin
                           SW_1_Pin */
  GPIO_InitStruct.Pin = SW_C_Pin|SW_D_Pin|SW_E_Pin|SW_2_Pin
                          |SW_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_D_Pin LED_2_Pin */
  GPIO_InitStruct.Pin = LED_D_Pin|LED_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_E_Pin LED_5_Pin LED_4_Pin LED_3_Pin
                           LED_1_Pin LED_A_Pin */
  GPIO_InitStruct.Pin = LED_E_Pin|LED_5_Pin|LED_4_Pin|LED_3_Pin
                          |LED_1_Pin|LED_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : SW_5_Pin SW_4_Pin SW_3_Pin SW_A_Pin */
  GPIO_InitStruct.Pin = SW_5_Pin|SW_4_Pin|SW_3_Pin|SW_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/*
 * @brief This function can be called to display an error message on the screen,
 * if the display is on, before calling Error_Handler() to stop all operations.
 */
void Error(char *msg) {
	if (ssd1306_GetDisplayOn() == 0) {
		/* Display is on */
		ssd1306_Fill(Black);
		ssd1306_SetCursor(2, 0);
		ssd1306_WriteString("Error:", Font_6x8, White);
		ssd1306_SetCursor(2, 9);
		ssd1306_WriteString(msg, Font_6x8, White);
		ssd1306_UpdateScreen();
	}

	Error_Handler();
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

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
