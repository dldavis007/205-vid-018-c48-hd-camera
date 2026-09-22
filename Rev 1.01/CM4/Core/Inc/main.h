/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "eeprom.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);
void MX_GPIO_Init(void);

/* USER CODE BEGIN EFP */


/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MUX_S2_Pin GPIO_PIN_6
#define MUX_S2_GPIO_Port GPIOB
#define MUX_S1_Pin GPIO_PIN_3
#define MUX_S1_GPIO_Port GPIOB
#define MUX_S0_Pin GPIO_PIN_11
#define MUX_S0_GPIO_Port GPIOG
#define MUX_EN_Pin GPIO_PIN_4
#define MUX_EN_GPIO_Port GPIOE
#define MIC_SHDN_Pin GPIO_PIN_13
#define MIC_SHDN_GPIO_Port GPIOC
#define MIC_GAIN_Pin GPIO_PIN_3
#define MIC_GAIN_GPIO_Port GPIOA
#define WDI_Pin GPIO_PIN_2
#define WDI_GPIO_Port GPIOA
#define RELAY_Pin GPIO_PIN_5
#define RELAY_GPIO_Port GPIOH

/* USER CODE BEGIN Private defines */
void InitCANstack (void);
void InitVirtAddVarTab(void);
void COP_Trig(void);
void copy_menu_data_to_ram(void);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
