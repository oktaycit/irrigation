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
/* HSE_VALUE is defined in stm32f4xx_hal_conf.h */
#define HSE_BYPASS 0U
#define CLOCK_SOURCE RCC_PLLSOURCE_HSE
#define SYSCLK_FREQ 168000000U      /* 168 MHz System Clock */
#define HCLK_FREQ SYSCLK_FREQ       /* AHB Clock */
#define PCLK1_FREQ (HCLK_FREQ / 4U) /* APB1 Clock (42 MHz) */
#define PCLK2_FREQ (HCLK_FREQ / 2U) /* APB2 Clock (84 MHz) */

/* Memory Map ----------------------------------------------------------------*/
/* FLASH_BASE and SRAM_BASE are defined in CMSIS device header */
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

/* Board bring-up diagnostics -----------------------------------------------*/
/*
 * This board exposes visible user LEDs on PA6/PA7. Enable this mode to build
 * a minimal firmware that only blinks D2/D3 so we can confirm the MCU is
 * executing code during board bring-up.
 */
#define BOARD_LED_DIAGNOSTIC_MODE 0U

/*
 * Minimal TFT test mode. Only initializes the LCD path and cycles solid colors
 * on the screen, which helps isolate FSMC/panel issues from the full app.
 */
#define BOARD_LCD_TEST_MODE 0U

/*
 * LCD probe mode. Reads controller ID/status registers and leaves the raw
 * values in RAM so we can inspect them over SWD without needing UART output.
 */
#define BOARD_LCD_PROBE_MODE 0U

/*
 * LCD sweep mode. Tries a set of common 320x240 parallel-TFT controller/init
 * combinations and control-line assumptions so we can identify a working
 * profile just by watching the panel.
 */
#define BOARD_LCD_SWEEP_MODE 0U

/*
 * Short irrigation demo. Runs a one-shot valve sequence after boot so we can
 * verify the output stages and GUI updates without relying on sensor inputs.
 */
#define BOARD_IRRIGATION_DEMO_MODE 0U

/*
 * Sensor demo mode. Replaces live pH/EC readings with a small looping profile
 * so we can exercise the main screen without depending on external probes.
 */
#define BOARD_SENSOR_DEMO_MODE 1U

/* Cooperative task scheduler -----------------------------------------------*/
#define SYSTEM_TASK_PERIOD_MS 100U
#define SYSTEM_VALIDATION_INTERVAL 10U
#define SYSTEM_SENSOR_STALE_TIMEOUT_MS 3000U
#define SYSTEM_MAINTENANCE_PERIOD_MS 1000U
#define SYSTEM_STATUS_PERIOD_MS 250U
#define SYSTEM_RUNTIME_SNAPSHOT_PERIOD_MS 2000U
#define SYSTEM_SCHEDULE_GUARD_MINUTE 1U

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
  uint8_t rtc_ok;
  uint8_t eeprom_crc_ok;
  uint8_t error_code;
  char alarm_text[32];
} SystemStatus_t;

extern SystemStatus_t gSystemStatus;

/* Function Prototypes ------------------------------------------------------*/
void System_Init(void);
void System_Status_Update(void);
void System_CommunicationTask(void);
void System_HMITask(void);
void System_ProgramManagementTask(void);
void System_IrrigationTask(void);
void System_SafetyMonitoringTask(void);
void System_MaintenanceTask(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_CONFIG_H */
