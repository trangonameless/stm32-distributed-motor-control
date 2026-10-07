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
#include "adc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "motor_control.h"
#include "current_sense.h"
#include "speed_sensor.h"
#include "telemetry.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pi_controller.h>
#include <uart_interface.h>
#include "diagnostics.h"
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
//static float target_rpm = 0.0f;
//static uint8_t motor_enabled = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t rx_byte;

/*
 * Retargets standard C library output (printf) to USART2.
 * Required for any printf()-based debug/telemetry logging to actually
 * appear on the wire — without this, printf() calls elsewhere silently
 * do nothing (or behave per whatever default newlib backend is linked).
 */
int __io_putchar(int ch)
{
    if (ch == '\n')
    {
        uint8_t ch2 = '\r';
        HAL_UART_Transmit(&huart2, &ch2, 1, HAL_MAX_DELAY);
    }
    HAL_UART_Transmit(&huart2, (uint8_t*) &ch, 1, HAL_MAX_DELAY);
    return 1;
}

/*
 * USART1 RX interrupt callback: fires once per received byte (single-byte
 * reception is re-armed at the end, so this must stay fast and non-blocking).
 * Bytes are handed off to the command-line assembler in uart_interface.c;
 * actual command parsing happens later in the main loop via
 * UART_Command_Get(), not here in interrupt context.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        UART_Command_ReceiveByte(rx_byte);

        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
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
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
  /* USER CODE BEGIN 2 */
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);    // start motor PWM channel
    CurrentSense_Init();
    SpeedSensor_Init();
    PI_Init(1.1f, 0.4f);                        // kp = 1.1, ki = 0.4 (PI controller, no derivative term)
    Diagnostics_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    char command[LINE_MAX_LENGTH + 1];

    while (1)
    {
        static uint32_t timer     = 0;   // 250ms telemetry period
        static uint32_t pid_timer = 0;   // 10ms control-loop period

        /* --- UART command reception --- */
        // Non-blocking: only processes a command once a full line has
        // been assembled (terminated by \r or \n) in uart_interface.c.
        if (UART_Command_Get(command))
        {
            Process_Command(command);
        }

        // Poll the encoder counter and refresh the RPM estimate.
        // Internally rate-limited to UPDATE_PERIOD_MS (10ms), so calling
        // it every loop iteration is safe and keeps the reading current.
        SpeedSensor_Update();

        /* --- Control loop: runs every 10ms --- */
        if (HAL_GetTick() - pid_timer >= 10)
        {
            pid_timer = HAL_GetTick();

            // Speed sensor reports signed RPM (direction implicit in
            // encoder count direction); PID operates on magnitude only,
            // since direction is set separately via Motor_SetDirection().
            float rpm = fabsf(SpeedSensor_GetRPM());
            float pwm = 0.0f;

            if (motor_enabled)
            {
                pwm = PI_Update(target_rpm, rpm);
                Motor_SetSpeed((uint8_t) pwm);
            }
            // Diagnostics run unconditionally, even when the motor is disabled.
            // This matters specifically for Driver_Fault() (DRV8876 nFAULT pin):
            // it's a hardware protection signal (overcurrent/overvoltage) that
            // can assert regardless of whether we're actively commanding the
            // motor, so it must not be gated behind motor_enabled. The other
            // three checks (stall/encoder/tracking) have their own threshold
            // conditions that naturally stay inactive when pwm=0/setpoint=0,
            // so running them unconditionally here is harmless.
            Diagnostics_Update(target_rpm, rpm, pwm);

            uint8_t faults = Diagnostics_GetFaults();

            if (faults != MOTOR_FAULT_NONE)
            {
                // Fail-safe: any active fault immediately stops the motor
                // and disables control until explicitly cleared via the
                // 'R' UART command (Diagnostics_ClearFaults()).
                Motor_Stop();
                motor_enabled = false;
                printf("Faults = 0x%02X\r\n", faults);
            }
        }

        /* --- Telemetry: runs every 250ms --- */
        if (HAL_GetTick() - timer >= 250)
        {
            timer = HAL_GetTick();

            float current = CurrentSense_ReadCurrent();
            float rpm = fabsf(SpeedSensor_GetRPM());

            // Clamp before casting to uint16_t: target_rpm should already be
            // non-negative (enforced in Process_Command), but this keeps the
            // telemetry packing safe even if that invariant is ever broken
            // elsewhere in the code.
            float safe_target_rpm = (target_rpm < 0.0f) ? 0.0f : target_rpm;
            uint16_t set_speed = (uint16_t) safe_target_rpm;

            uint8_t pwm = Motor_GetPWM();

            Telemetry_SendData(current, rpm, pwm, set_speed);
        }
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
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
	while (1) {
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
