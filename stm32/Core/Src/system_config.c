/**
 ******************************************************************************
 * @file           : system_config.c
 * @brief          : System configuration and cooperative task entrypoints
 ******************************************************************************
 */

#include "main.h"
#include <stdio.h>
#include <string.h>

SystemStatus_t gSystemStatus = {0};

void System_Init(void) {
  gSystemStatus = (SystemStatus_t){0};

  CRC_Init();
  gSystemStatus.rtc_ok = RTC_Init();
  LOW_POWER_Init();

  LCD_Init();
  gSystemStatus.lcd_ok = 1U;

  if (EEPROM_Init() == EEPROM_OK) {
    gSystemStatus.eeprom_ok = 1U;
    gSystemStatus.eeprom_crc_ok = 1U;
  }

  TOUCH_Init();
  gSystemStatus.touch_ok = 1U;

  GUI_Init();
  LCD_SetBacklight(80);
  BUZZER_Init();

  SENSORS_Init();
  SENSORS_StartContinuous();
  VALVES_Init();
  IRRIGATION_CTRL_Init();
  USB_CONFIG_Init();

  GUI_NavigateTo(SCREEN_MAIN);
  gSystemStatus.system_ready = 1U;
  BUZZER_BeepSuccess();
}

void System_Status_Update(void) {
  snprintf(gSystemStatus.alarm_text, sizeof(gSystemStatus.alarm_text), "%s",
           IRRIGATION_CTRL_GetActiveAlarmText());

  gSystemStatus.ph_sensor_ok = (PH_GetStatus() == SENSOR_OK) ? 1U : 0U;
  gSystemStatus.ec_sensor_ok = (EC_GetStatus() == SENSOR_OK) ? 1U : 0U;
  gSystemStatus.rtc_ok = RTC_IsInitialized();
  gSystemStatus.eeprom_crc_ok =
      (EEPROM_GetLastError() == EEPROM_CRC_ERROR) ? 0U : 1U;

  if (IRRIGATION_CTRL_HasErrors() != 0U) {
    gSystemStatus.error_code = (uint8_t)IRRIGATION_CTRL_GetLastError();
  } else if (gSystemStatus.ph_sensor_ok == 0U || gSystemStatus.ec_sensor_ok == 0U ||
             gSystemStatus.rtc_ok == 0U || gSystemStatus.eeprom_crc_ok == 0U) {
    gSystemStatus.error_code = 1U;
  } else {
    gSystemStatus.error_code = 0U;
  }
}

void System_CommunicationTask(void) {
  SENSORS_Process();
  USB_CONFIG_Process();
}

void System_HMITask(void) { GUI_Update(); }

void System_ProgramManagementTask(void) { IRRIGATION_CTRL_CheckSchedules(); }

void System_IrrigationTask(void) {
  VALVES_Update();
  IRRIGATION_CTRL_Update();
}

void System_SafetyMonitoringTask(void) {
  (void)IRRIGATION_CTRL_RunSafetyChecks();
  System_Status_Update();
}

void System_MaintenanceTask(void) { IRRIGATION_CTRL_MaintenanceTask(); }
