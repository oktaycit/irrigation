/**
 ******************************************************************************
 * @file           : irrigation_alarm_coordinator.c
 * @brief          : Alarm/fault akisi icin orkestrasyon yardimcilari
 ******************************************************************************
 */

#include "irrigation_alarm_coordinator.h"
#include "dosing_controller.h"
#include "fault_manager.h"
#include "irrigation_runtime.h"
#include "valves.h"

void IRRIGATION_ALARM_Sync(irrigation_control_t *ctrl) {
  if (ctrl == NULL) {
    return;
  }

  ctrl->error_flags = FAULT_MGR_HasActiveFault();
  ctrl->last_error = FAULT_MGR_GetLastError();
}

void IRRIGATION_ALARM_RaiseError(irrigation_control_t *ctrl,
                                 error_code_t error,
                                 void (*change_state)(control_state_t),
                                 void (*on_error)(error_code_t)) {
  if (ctrl == NULL || error == ERR_NONE) {
    return;
  }

  FAULT_MGR_Raise(error, 1U, 1U, CTRL_STATE_ERROR);
  IRRIGATION_ALARM_Sync(ctrl);
  ctrl->is_running = 0U;
  ctrl->is_paused = 0U;
  DOSING_CTRL_Reset();
  VALVES_EmergencyStop();
  IRRIGATION_RUNTIME_SetProgramState(PROGRAM_STATE_ERROR);
  if (change_state != NULL) {
    change_state(CTRL_STATE_ERROR);
  }
  IRRIGATION_RUNTIME_RequestPersist();
  if (on_error != NULL) {
    on_error(error);
  }
}

void IRRIGATION_ALARM_Clear(irrigation_control_t *ctrl, error_code_t error) {
  FAULT_MGR_Clear(error);
  IRRIGATION_ALARM_Sync(ctrl);
}

void IRRIGATION_ALARM_ClearAll(irrigation_control_t *ctrl) {
  FAULT_MGR_ClearAll();
  VALVES_ClearErrors();
  DOSING_CTRL_Reset();
  IRRIGATION_ALARM_Sync(ctrl);
}

void IRRIGATION_ALARM_EnterEmergencyStop(
    irrigation_control_t *ctrl,
    error_code_t fallback_error,
    void (*change_state)(control_state_t)) {
  if (ctrl == NULL) {
    return;
  }

  DOSING_CTRL_Reset();
  VALVES_EmergencyStop();
  ctrl->is_running = 0U;
  FAULT_MGR_EnterEmergencyStop(fallback_error);
  IRRIGATION_ALARM_Sync(ctrl);
  IRRIGATION_RUNTIME_SetProgramState(PROGRAM_STATE_ERROR);
  if (change_state != NULL) {
    change_state(CTRL_STATE_EMERGENCY_STOP);
  }
  IRRIGATION_RUNTIME_RequestPersist();
}

uint8_t IRRIGATION_ALARM_Acknowledge(irrigation_control_t *ctrl) {
  if (FAULT_MGR_HasActiveFault() == 0U) {
    return 1U;
  }

  FAULT_MGR_Acknowledge();
  IRRIGATION_ALARM_Sync(ctrl);
  return 1U;
}

uint8_t IRRIGATION_ALARM_Reset(
    irrigation_control_t *ctrl,
    void (*stop_handler)(void),
    uint8_t (*safety_check_handler)(void)) {
  if (FAULT_MGR_HasActiveFault() == 0U) {
    return 1U;
  }

  if (FAULT_MGR_CanReset() == 0U) {
    IRRIGATION_ALARM_Sync(ctrl);
    return 0U;
  }

  if (stop_handler != NULL) {
    stop_handler();
  }

  VALVES_ClearErrors();
  DOSING_CTRL_Reset();
  FAULT_MGR_ClearAll();
  IRRIGATION_ALARM_Sync(ctrl);

  if (safety_check_handler != NULL && safety_check_handler() == 0U &&
      FAULT_MGR_HasActiveFault() != 0U) {
    IRRIGATION_ALARM_Sync(ctrl);
    return 0U;
  }

  IRRIGATION_ALARM_Sync(ctrl);
  return (FAULT_MGR_HasActiveFault() == 0U) ? 1U : 0U;
}
