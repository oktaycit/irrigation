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

/* ADC DMA Buffer Configuration */
#define ADC_DMA_BUFFER_SIZE     2U    /* pH ve EC için 2 kanal */
#define ADC_DMA_CIRCULAR_MODE   1U

/* System tick definitions */
#define SYSTEM_TICK_FREQ_HZ     1000U  /* 1ms base */

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

void Error_Handler(void);

/* External variables */
extern volatile uint16_t g_adc_dma_buffer[ADC_DMA_BUFFER_SIZE];
extern volatile uint32_t g_system_tick;

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
#include "usb_device.h"
#include "usb_config.h"
#include "buzzer.h"
#include "hw_crc.h"
#include "rtc_driver.h"

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
