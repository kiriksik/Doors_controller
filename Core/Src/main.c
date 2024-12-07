/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define MAX_MESSAGE_LENGTH 8
#define ROWS 4
#define COLS 4
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
bool alarm = false;
bool mega_alarm = false;
bool blocked = false;
volatile uint8_t alarm_counter = 0;
volatile uint8_t shift = 15;
volatile char aprd[MAX_MESSAGE_LENGTH] = "aprd";
volatile char deny[MAX_MESSAGE_LENGTH] = "deny";
volatile char buff[MAX_MESSAGE_LENGTH];
const char alarm_code[4] = "1234";
volatile char input_code[4] = "mmmm";
const char *validCodes[] = {
    "2345",
    "1730",
    "5344",
    "1234",
	"1111"
};
volatile char input[MAX_MESSAGE_LENGTH];
volatile char output[MAX_MESSAGE_LENGTH];
volatile char decrypted[MAX_MESSAGE_LENGTH];
const uint8_t validCodesCount = sizeof(validCodes) / sizeof(validCodes[0]);
volatile uint8_t doors[validCodesCount] = {0, 0, 0, 0, 0};
GPIO_TypeDef* rowPorts[ROWS] = {GPIOB, GPIOB, GPIOB, GPIOB};
uint16_t rowPins[ROWS] = {GPIO_PIN_7, GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10};
GPIO_TypeDef* colPorts[COLS] = {GPIOA, GPIOA, GPIOB, GPIOB};
uint16_t colPins[COLS] = {GPIO_PIN_11, GPIO_PIN_12, GPIO_PIN_5, GPIO_PIN_6};
uint16_t diods[4] = {GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7};
uint16_t alarms[3] = {GPIO_PIN_15, GPIO_PIN_14, GPIO_PIN_13};
char keyMap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

char scanKeypad(void) {
    int activeRows = 0; // Счётчик активных строк
    int activeRowIndex = -1;
    int activeColIndex = -1;

    for (int col = 0; col < COLS; col++) {
        // Установить все столбцы в HIGH
        for (int i = 0; i < COLS; i++) {
            HAL_GPIO_WritePin(colPorts[i], colPins[i], GPIO_PIN_SET);
        }

        // Установить текущий столбец в LOW
        HAL_GPIO_WritePin(colPorts[col], colPins[col], GPIO_PIN_RESET);

        for (int row = 0; row < ROWS; row++) {
            if (HAL_GPIO_ReadPin(rowPorts[row], rowPins[row]) == GPIO_PIN_RESET) {
                HAL_Delay(100);
                if (HAL_GPIO_ReadPin(rowPorts[row], rowPins[row]) == GPIO_PIN_RESET) {
                    activeRows++;
                    activeRowIndex = row;
                    activeColIndex = col;


                    if (activeRows > 1) {
                        return 0;
                    }
                }
            }
        }
    }

    if (activeRows == 1) {
        return keyMap[activeRowIndex][activeColIndex];
    }

    return 0;
}



uint8_t isValidCode(const char *code) {
    for (uint8_t i = 0; i < validCodesCount; i++) {
        if (strncmp(code, validCodes[i], 4) == 0) {
        	if (doors[i] == 1)
        		doors[i] = 1;
        	else
        		doors[i] = 0;
            return 1;
        }
    }
    return 0;
}


uint8_t check_len(const char* str) {
	uint8_t len = 0;
	while (1) {
		if (len>20)
			return 0;
		if (str[len] == '\0')
			return len;
		else
			len++;
	}
}


bool check_key(const char* str) {
    uint8_t len = check_len(str);
    uint8_t first_char = str[len-2] - 48;
    uint8_t second_char = str[len-1] - 48;
    uint8_t first_digit = shift / 10;
    uint8_t second_digit = shift % 10;
    if (first_char == first_digit && second_char == second_digit)
    	return true;
    return false;
}


void decrypt(const char* message, char* output) {
    uint8_t len = check_len(message);
    for (size_t i = 0; i < len; i++) {
        output[i] = message[i] - shift;
    }
    output[len]= '\0';
//    shift = (shift-9) % 20 + 10;
}


void encrypt(const char* message, char* output) {
    uint8_t len = check_len(message);
    for (size_t i = 0; i < len+3; i++) {
    	output[i] = ((message[i] + shift - 32) % 96) + 32;
    }
    output[len] = shift / 10 + '0'  + shift;
    output[len + 1] = shift % 10 + '0' + shift;
    output[len + 2]= '\0';
    shift = (shift-9) % 20 + 10;

}


void check_alarm_code(const char* str) {
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
}


void USART2_SendString(const char* str) {
    uint8_t len = check_len(str);
    HAL_UART_Transmit(&huart2, (uint8_t*)str, len+1, 1000);
//	HAL_Delay(1000);
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
//	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
//	HAL_Delay(1000);
//	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
	if(huart == &huart2)
	{


		decrypt(buff, decrypted);

		// проверка на корректность смещения
		if (!check_key(decrypted)) {
			alarm = true;
			encrypt(deny, output);
			USART2_SendString(output);
//			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		}
		else {
			// обработка
			if (isValidCode((const char *)decrypted)) {
				// Код корректный
				encrypt(aprd, output);
				USART2_SendString(output);
			} else {
				// Код некорректный
				encrypt(deny, output);
				USART2_SendString(output);
				alarm = true;

			}
		}


//		HAL_UART_Receive_IT(&huart2, (uint8_t*)buff, 15);
//		HAL_UART_Receive_IT(&huart2, (uint8_t*)buff, 15);
	}
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
  char keyPressed;
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */


  while (1)
  {
	  if (!alarm) {
		  if (HAL_UART_GetState (&huart2) == HAL_UART_STATE_READY)
		  {
			  HAL_UART_Receive_IT(&huart2, (uint8_t*)buff, 7);
	//		  HAL_Delay(100);

		  }
	  }
	  else {
		  if (!mega_alarm) {
		  	for (int i = 0; i < 4; i++) {
		  		if (input_code[i] != 'm')
		  			HAL_GPIO_WritePin(GPIOA, diods[i], GPIO_PIN_SET);
		  		else
		  			HAL_GPIO_WritePin(GPIOA, diods[i], GPIO_PIN_RESET);
		  	}
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
			keyPressed = scanKeypad();
				if (keyPressed) {
					if (!blocked) {
						if (keyPressed == '0' || keyPressed == '1' || keyPressed == '2' || keyPressed == '3' || keyPressed == '4' || keyPressed == '5' || keyPressed == '6' || keyPressed == '7' || keyPressed == '8' || keyPressed == '9') {
							for (int i = 0; i < 4; i++) {
								if (input_code[i] == 'm') {
									input_code[i] = keyPressed;
									break;
								}
							}
						}
						else if (keyPressed == '*') {
							if (input_code[3] != 'm')
								input_code[3] = 'm';
							else if (input_code[2] != 'm')
								input_code[2] = 'm';
							else if (input_code[1] != 'm')
								input_code[1] = 'm';
							else if (input_code[0] != 'm')
								input_code[0] = 'm';

						}
						else if (keyPressed == '#') {
							if (input_code[3] != 'm') {
								// проверка кода
								int count = 0;
								for (int i = 0; i < 4; i++) {
									if (input_code[i] == alarm_code[i])
										count++;
								}
								if (count == 4) {
									HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
									for (int i = 0; i < 3; i++)
										HAL_GPIO_WritePin(GPIOB, alarms[i], GPIO_PIN_RESET);
									alarm_counter = 0;
									alarm = false;
									for (int i = 0; i < 4; i++)
										HAL_GPIO_WritePin(GPIOA, diods[i], GPIO_PIN_RESET);
									encrypt(aprd, output);
									USART2_SendString(output);
								}
								else {
									alarm_counter++;
									HAL_GPIO_WritePin(GPIOB, alarms[alarm_counter - 1], GPIO_PIN_SET);
									if (alarm_counter > 2) {
										mega_alarm = true;

									}
									for (int i = 0; i < 4; i++)
										HAL_GPIO_WritePin(GPIOA, diods[i], GPIO_PIN_RESET);
								}
								for (int i = 0; i < 4; i++) {
									input_code[i] = 'm';
								}
							}


						}
						else if (keyPressed == 'B') {
							blocked = true;
						}
						else if (keyPressed == 'C') {
							for (int i = 0; i < 4; i++) {
								input_code[i] = 'm';
							}
						}
						else if (keyPressed == 'D') {
							HAL_NVIC_SystemReset();
						}

					}
					else if (keyPressed == 'A')
						blocked = false;
					else if (keyPressed == 'D') {
						HAL_NVIC_SystemReset();
					}
			}
		  }
	  }


//	  while( HAL_UART_GetState (&huart2) == HAL_UART_STATE_BUSY_TX );
//	  HAL_Delay(1000);



//	  decrypt(output, decrypted);
//	  USART2_SendString(decrypted);
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
  huart2.Init.BaudRate = 9600;
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

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_5
                          |GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA4 PA5 PA6 PA7
                           PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB10 PB7 PB8 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB13 PB14 PB15 PB5
                           PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_5
                          |GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
