/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   STM32F407VET6 Sulama Sistemi
 ******************************************************************************
 */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

void Error_Handler(void);

/* HAL TIM Callbacks ------------------------------------------------------- */
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* System Includes -----------------------------------------------------------*/
#include "eeprom.h"
#include "gui.h"
#include "lcd_ili9341.h"
#include "sensors.h"
#include "system_config.h"
#include "touch_xpt2046.h"
#include "valves.h"
#include "irrigation_control.h"

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
