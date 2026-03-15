/**
 ******************************************************************************
 * @file           : system_config.h
 * @brief          : Sistem konfigürasyon ayarları
 ******************************************************************************
 */

#ifndef __SYSTEM_CONFIG_H
#define __SYSTEM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

/* System Clock Configuration -----------------------------------------------*/
#define HSE_VALUE ((uint32_t)8000000U) /* 8 MHz External oscillator */
#define HSE_BYPASS 0U
#define CLOCK_SOURCE RCC_CLOCKTYPE_HSE
#define SYSCLK_FREQ 168000000U      /* 168 MHz System Clock */
#define HCLK_FREQ SYSCLK_FREQ       /* AHB Clock */
#define PCLK1_FREQ (HCLK_FREQ / 4U) /* APB1 Clock (42 MHz) */
#define PCLK2_FREQ (HCLK_FREQ / 2U) /* APB2 Clock (84 MHz) */

/* Memory Map ----------------------------------------------------------------*/
#define FLASH_BASE 0x08000000U
#define SRAM_BASE 0x20000000U
#define FLASH_SIZE 512U /* 512 KB */
#define SRAM_SIZE 192U  /* 192 KB */

/* Interrupt Priorities -----------------------------------------------------*/
#define NVIC_PRIORITYGROUP NVIC_PRIORITYGROUP_4

#define TOUCH_IRQ_PRIORITY 0U /* En yüksek öncelik */
#define ADC_IRQ_PRIORITY 1U
#define TIM3_IRQ_PRIORITY 2U
#define USART1_IRQ_PRIORITY 3U
#define I2C1_IRQ_PRIORITY 4U

/* Tick Configuration -------------------------------------------------------*/
#define SYSTICK_PERIOD_MS 1U
#define SYSTICK_FREQ 1000U /* 1 kHz */

/* Watchdog Configuration ---------------------------------------------------*/
#define IWDG_TIMEOUT_MS 2000U /* 2 saniye */
#define WWDG_TIMEOUT_MS 500U  /* 500 ms */

/* Debug Configuration ------------------------------------------------------*/
#define DEBUG_ENABLED 1U
#define DEBUG_UART USART1
#define DEBUG_BAUDRATE 115200U

/* Version Information ------------------------------------------------------*/
#define FIRMWARE_VERSION_MAJOR 0U
#define FIRMWARE_VERSION_MINOR 1U
#define FIRMWARE_VERSION_PATCH 0U
#define FIRMWARE_VERSION "0.1.0"
#define FIRMWARE_BUILD_DATE __DATE__
#define FIRMWARE_BUILD_TIME __TIME__

/* System Status Flags ------------------------------------------------------*/
typedef struct {
  uint8_t system_ready;
  uint8_t ph_sensor_ok;
  uint8_t ec_sensor_ok;
  uint8_t lcd_ok;
  uint8_t touch_ok;
  uint8_t eeprom_ok;
  uint8_t error_code;
} SystemStatus_t;

extern SystemStatus_t gSystemStatus;

/* Function Prototypes ------------------------------------------------------*/
void System_Init(void);
void System_Status_Update(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_CONFIG_H */