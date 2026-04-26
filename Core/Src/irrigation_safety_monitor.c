/**
 ******************************************************************************
 * @file           : irrigation_safety_monitor.c
 * @brief          : Sulama guvenlik izleme ve hata siniflandirma yardimcilari
 ******************************************************************************
 */

#include "irrigation_safety_monitor.h"
#include "dosing_controller.h"
#include "rtc_driver.h"
#include "sensors.h"
#include "system_config.h"
#include "valves.h"
#include "stm32f4xx_hal.h"
#include <string.h>

void IRRIGATION_SAFETY_Evaluate(uint8_t current_valve_id,
                                irrigation_safety_result_t *result) {
  uint32_t now;
  uint8_t valve_error_mask;
  uint16_t dosing_active_mask;

  if (result == NULL) {
    return;
  }

  memset(result, 0, sizeof(*result));
  result->healthy = 1U;
  result->error = ERR_NONE;

  now = HAL_GetTick();
  valve_error_mask = VALVES_GetErrorMask();
  dosing_active_mask = DOSING_CTRL_GetActiveMask();

  result->valve_error_mask = valve_error_mask;

  if (RTC_IsInitialized() == 0U) {
    result->healthy = 0U;
    result->action = IRRIGATION_SAFETY_ACTION_ERROR;
    result->error = ERR_COMMUNICATION;
    return;
  }

  if ((now - PH_GetLastReadTime()) > SYSTEM_SENSOR_STALE_TIMEOUT_MS) {
    result->healthy = 0U;
    result->action = IRRIGATION_SAFETY_ACTION_ERROR;
    result->error = ERR_PH_SENSOR_FAULT;
    return;
  }

  if ((now - EC_GetLastReadTime()) > SYSTEM_SENSOR_STALE_TIMEOUT_MS) {
    result->healthy = 0U;
    result->action = IRRIGATION_SAFETY_ACTION_ERROR;
    result->error = ERR_EC_SENSOR_FAULT;
    return;
  }

  if ((valve_error_mask & VALVE_FAULT_EMERGENCY_STOP) != 0U) {
    result->healthy = 0U;
    result->action = IRRIGATION_SAFETY_ACTION_EMERGENCY_STOP;
    result->error = ERR_TIMEOUT;
    return;
  }

  if ((valve_error_mask & (VALVE_FAULT_DRIVER | VALVE_FAULT_TIMEOUT)) != 0U) {
    result->healthy = 0U;
    result->action = IRRIGATION_SAFETY_ACTION_ERROR;
    result->error = ERR_COMMUNICATION;
    return;
  }

  if ((valve_error_mask &
       (VALVE_FAULT_STATE_MISMATCH | VALVE_FAULT_PARCEL_INTERLOCK |
        VALVE_FAULT_DOSING_INTERLOCK | VALVE_FAULT_INVALID_ID |
        VALVE_FAULT_DISABLED)) != 0U) {
    result->healthy = 0U;
    result->action = IRRIGATION_SAFETY_ACTION_ERROR;
    result->error = ERR_VALVE_STUCK;
    return;
  }

  if (dosing_active_mask != 0U && current_valve_id != 0U &&
      ((dosing_active_mask & (uint16_t)(1U << (current_valve_id - 1U))) != 0U)) {
    result->healthy = 0U;
    result->action = IRRIGATION_SAFETY_ACTION_ERROR;
    result->error = ERR_VALVE_STUCK;
    return;
  }
}
