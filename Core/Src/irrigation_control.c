/**
 ******************************************************************************
 * @file           : irrigation_control.c
 * @brief          : Program-based irrigation control implementation
 ******************************************************************************
 */

#include "main.h"
#include "dosing_controller.h"
#include "fault_manager.h"
#include "irrigation_alarm_coordinator.h"
#include "irrigation_persistence.h"
#include "irrigation_run_session.h"
#include "irrigation_safety_monitor.h"
#include "irrigation_runtime.h"
#include "irrigation_schedule_trigger.h"
#include "parcel_scheduler.h"
#include "irrigation_control.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static irrigation_control_t ctrl = {0};
static auto_mode_t g_auto_mode = AUTO_MODE_SCHEDULED;
static irrigation_schedule_t g_schedules[IRRIGATION_PROGRAM_COUNT] = {0};
static irrigation_program_t g_programs[IRRIGATION_PROGRAM_COUNT] = {0};
static irrigation_runtime_backup_t g_runtime_backup = {0};
static eeprom_system_t g_system_record = {0};

static void IRRIGATION_CTRL_ApplySystemRecord(void);
static void IRRIGATION_CTRL_SetDefaults(void);
static auto_mode_t IRRIGATION_CTRL_GetStoredAutoMode(void);
static void IRRIGATION_CTRL_StoreAutoMode(auto_mode_t mode);
static void IRRIGATION_CTRL_PersistRuntimeBackup(void);
static void IRRIGATION_CTRL_ClearRuntimeBackup(void);
static void IRRIGATION_CTRL_ChangeState(control_state_t new_state);
static void IRRIGATION_CTRL_FinishRun(void);
static uint8_t IRRIGATION_CTRL_StartPreparedRun(void);
static uint8_t IRRIGATION_CTRL_StartCurrentValveCycle(void);
static uint8_t IRRIGATION_CTRL_IsRestorableState(program_state_t state);
static void IRRIGATION_CTRL_AbortRuntimeRestore(error_code_t error);
static program_state_t IRRIGATION_CTRL_GetInitialValvePhase(void);
static control_state_t IRRIGATION_CTRL_GetControlStateForProgramPhase(
    program_state_t phase);
static void IRRIGATION_CTRL_HandleCompletedValveStep(
    irrigation_run_step_t run_step);
static void IRRIGATION_CTRL_ServiceFlushPhase(void);
static void IRRIGATION_CTRL_ServiceValveActive(void);
static void IRRIGATION_CTRL_ServiceWaiting(void);
static void IRRIGATION_CTRL_ServiceDosing(void);
static void IRRIGATION_CTRL_UpdateRuntimeBackup(void);
static void IRRIGATION_CTRL_NormalizeProgram(irrigation_program_t *program);
static void IRRIGATION_CTRL_NormalizeProgramRecipe(irrigation_program_t *program);

void IRRIGATION_CTRL_Init(void) {
  memset(&ctrl, 0, sizeof(ctrl));
  memset(&g_runtime_backup, 0, sizeof(g_runtime_backup));
  memset(g_schedules, 0, sizeof(g_schedules));
  memset(g_programs, 0, sizeof(g_programs));

  IRRIGATION_CTRL_SetDefaults();
  DOSING_CTRL_Init();
  FAULT_MGR_Init();
  IRRIGATION_PERSIST_Init();
  IRRIGATION_RUNTIME_Init();
  PARCEL_SCHED_Init();
  IRRIGATION_CTRL_ApplySystemRecord();
  IRRIGATION_CTRL_LoadPrograms();
  IRRIGATION_CTRL_ChangeState(CTRL_STATE_IDLE);
  IRRIGATION_CTRL_LoadRuntimeBackup();
}

void IRRIGATION_CTRL_SetAutoMode(auto_mode_t mode) {
  g_auto_mode = mode;
  IRRIGATION_CTRL_StoreAutoMode(mode);
  IRRIGATION_PERSIST_QueueSystemRecord(&g_system_record);
  IRRIGATION_PERSIST_Service();
}

auto_mode_t IRRIGATION_CTRL_GetAutoMode(void) { return g_auto_mode; }

void IRRIGATION_CTRL_SetPHParams(float target, float min, float max, float hyst) {
  if (min > max) {
    float temp = min;
    min = max;
    max = temp;
  }

  if (target < min) target = min;
  if (target > max) target = max;

  ctrl.ph_params.target = target;
  ctrl.ph_params.min_limit = min;
  ctrl.ph_params.max_limit = max;
  if (hyst > 0.0f) {
    ctrl.ph_params.hysteresis = hyst;
  }

  g_system_record.ph_target = target;
  g_system_record.ph_min = min;
  g_system_record.ph_max = max;
  IRRIGATION_PERSIST_QueueSystemRecord(&g_system_record);
}

void IRRIGATION_CTRL_SetECParams(float target, float min, float max, float hyst) {
  if (min > max) {
    float temp = min;
    min = max;
    max = temp;
  }

  if (target < min) target = min;
  if (target > max) target = max;

  ctrl.ec_params.target = target;
  ctrl.ec_params.min_limit = min;
  ctrl.ec_params.max_limit = max;
  if (hyst > 0.0f) {
    ctrl.ec_params.hysteresis = hyst;
  }

  g_system_record.ec_target = target;
  g_system_record.ec_min = min;
  g_system_record.ec_max = max;
  IRRIGATION_PERSIST_QueueSystemRecord(&g_system_record);
}

void IRRIGATION_CTRL_SetPHDosingDuty(uint8_t duty_percent) {
  if (duty_percent > 100U) {
    duty_percent = 100U;
  }

  ctrl.ph_params.pwm_duty_percent = duty_percent;
}

void IRRIGATION_CTRL_SetECDosingDuty(uint8_t duty_percent) {
  if (duty_percent > 100U) {
    duty_percent = 100U;
  }

  ctrl.ec_params.pwm_duty_percent = duty_percent;
}

void IRRIGATION_CTRL_SetPHDosingResponse(uint32_t feedback_delay_ms,
                                         uint8_t response_gain_percent,
                                         uint8_t max_correction_cycles) {
  if (response_gain_percent > 100U) {
    response_gain_percent = 100U;
  }
  if (response_gain_percent == 0U) {
    response_gain_percent = 100U;
  }
  if (feedback_delay_ms > 600000UL) {
    feedback_delay_ms = 600000UL;
  }
  if (max_correction_cycles > 10U) {
    max_correction_cycles = 10U;
  }

  ctrl.ph_params.feedback_delay_ms = feedback_delay_ms;
  ctrl.ph_params.response_gain_percent = response_gain_percent;
  ctrl.ph_params.max_correction_cycles = max_correction_cycles;
  g_system_record.ph_feedback_delay_ms = feedback_delay_ms;
  g_system_record.ph_response_gain_percent = response_gain_percent;
  g_system_record.ph_max_correction_cycles = max_correction_cycles;
  IRRIGATION_PERSIST_QueueSystemRecord(&g_system_record);
}

void IRRIGATION_CTRL_SetECDosingResponse(uint32_t feedback_delay_ms,
                                         uint8_t response_gain_percent,
                                         uint8_t max_correction_cycles) {
  if (response_gain_percent > 100U) {
    response_gain_percent = 100U;
  }
  if (response_gain_percent == 0U) {
    response_gain_percent = 100U;
  }
  if (feedback_delay_ms > 600000UL) {
    feedback_delay_ms = 600000UL;
  }
  if (max_correction_cycles > 10U) {
    max_correction_cycles = 10U;
  }

  ctrl.ec_params.feedback_delay_ms = feedback_delay_ms;
  ctrl.ec_params.response_gain_percent = response_gain_percent;
  ctrl.ec_params.max_correction_cycles = max_correction_cycles;
  g_system_record.ec_feedback_delay_ms = feedback_delay_ms;
  g_system_record.ec_response_gain_percent = response_gain_percent;
  g_system_record.ec_max_correction_cycles = max_correction_cycles;
  IRRIGATION_PERSIST_QueueSystemRecord(&g_system_record);
}

void IRRIGATION_CTRL_SetECRecipe(const uint8_t *ratio_percent, uint8_t count) {
  uint8_t ratio_sum = 0U;

  if (ratio_percent == NULL || count == 0U) {
    return;
  }

  if (count > IRRIGATION_EC_CHANNEL_COUNT) {
    count = IRRIGATION_EC_CHANNEL_COUNT;
  }

  for (uint8_t i = 0; i < count; i++) {
    ctrl.ec_params.recipe_ratio[i] = ratio_percent[i];
    ratio_sum = (uint8_t)(ratio_sum + ratio_percent[i]);
  }

  for (uint8_t i = count; i < IRRIGATION_EC_CHANNEL_COUNT; i++) {
    ctrl.ec_params.recipe_ratio[i] = 0U;
  }

  if (ratio_sum == 0U) {
    for (uint8_t i = 0; i < IRRIGATION_EC_CHANNEL_COUNT; i++) {
      ctrl.ec_params.recipe_ratio[i] = 25U;
    }
  }
}

void IRRIGATION_CTRL_GetECRecipe(uint8_t *ratio_percent, uint8_t count) {
  if (ratio_percent == NULL || count == 0U) {
    return;
  }

  if (count > IRRIGATION_EC_CHANNEL_COUNT) {
    count = IRRIGATION_EC_CHANNEL_COUNT;
  }

  memcpy(ratio_percent, ctrl.ec_params.recipe_ratio, count);
}

void IRRIGATION_CTRL_GetPHParams(ph_control_params_t *params) {
  if (params != NULL) {
    *params = ctrl.ph_params;
  }
}

void IRRIGATION_CTRL_GetECParams(ec_control_params_t *params) {
  if (params != NULL) {
    *params = ctrl.ec_params;
  }
}

void IRRIGATION_CTRL_SaveDosingResponseParams(void) {
  g_system_record.ph_feedback_delay_ms = ctrl.ph_params.feedback_delay_ms;
  g_system_record.ec_feedback_delay_ms = ctrl.ec_params.feedback_delay_ms;
  g_system_record.ph_response_gain_percent = ctrl.ph_params.response_gain_percent;
  g_system_record.ec_response_gain_percent = ctrl.ec_params.response_gain_percent;
  g_system_record.ph_max_correction_cycles = ctrl.ph_params.max_correction_cycles;
  g_system_record.ec_max_correction_cycles = ctrl.ec_params.max_correction_cycles;
  IRRIGATION_PERSIST_QueueSystemRecord(&g_system_record);
  IRRIGATION_PERSIST_Service();
}

void IRRIGATION_CTRL_SetParcelDuration(uint8_t parcel_id, uint32_t duration_sec) {
  if (duration_sec == 0U) {
    duration_sec = 30U;
  }

  PARCELS_SetDuration(parcel_id, duration_sec);
  (void)EEPROM_SaveParcel(parcel_id);
}

void IRRIGATION_CTRL_SetParcelEnabled(uint8_t parcel_id, uint8_t enabled) {
  if (parcel_id == 0U || parcel_id > IRRIGATION_PROGRAM_VALVE_COUNT) {
    return;
  }

  PARCELS_SetEnabled(parcel_id, enabled);
  (void)EEPROM_SaveParcel(parcel_id);
}

void IRRIGATION_CTRL_SetParcelConfig(uint8_t parcel_id, uint32_t duration_sec,
                                     uint8_t enabled) {
  if (parcel_id == 0U || parcel_id > IRRIGATION_PROGRAM_VALVE_COUNT) {
    return;
  }

  if (duration_sec == 0U) {
    duration_sec = 30U;
  }

  PARCELS_SetDuration(parcel_id, duration_sec);
  PARCELS_SetEnabled(parcel_id, enabled);
  (void)EEPROM_SaveParcel(parcel_id);
}

void IRRIGATION_CTRL_Start(void) {
  if (ctrl.is_running != 0U) {
    return;
  }

  if (FAULT_MGR_HasActiveFault() != 0U) {
    IRRIGATION_ALARM_Sync(&ctrl);
    return;
  }

  if (PARCEL_SCHED_GetQueueSize() == 0U) {
    for (uint8_t parcel_id = 1U; parcel_id <= IRRIGATION_PROGRAM_VALVE_COUNT;
         parcel_id++) {
      if (PARCELS_IsEnabled(parcel_id) != 0U) {
        IRRIGATION_CTRL_AddToQueue(parcel_id);
      }
    }
  }

  if (PARCEL_SCHED_BuildSequenceFromQueue() == 0U) {
    return;
  }

  PARCEL_SCHED_SetManualSequence(1U);
  PARCEL_SCHED_SetActiveProgram(0U);
  PARCEL_SCHED_SetRepeatIndex(0U);
  (void)IRRIGATION_CTRL_StartPreparedRun();
}

void IRRIGATION_CTRL_StartProgram(uint8_t program_id) {
  irrigation_program_t *program;

  if (program_id == 0U || program_id > IRRIGATION_PROGRAM_COUNT) {
    return;
  }

  if (FAULT_MGR_HasActiveFault() != 0U) {
    IRRIGATION_ALARM_Sync(&ctrl);
    return;
  }

  program = &g_programs[program_id - 1U];
  if (program->enabled == 0U) {
    return;
  }

  if (PARCEL_SCHED_BuildSequenceFromMask(program->valve_mask) == 0U) {
    return;
  }

  PARCEL_SCHED_SetManualSequence(0U);
  PARCEL_SCHED_SetActiveProgram(program_id);
  PARCEL_SCHED_SetActiveValveIndex(0U);
  PARCEL_SCHED_SetRepeatIndex(0U);
  IRRIGATION_RUN_ApplyProgramTargets(&ctrl, program);
  (void)IRRIGATION_CTRL_StartPreparedRun();
}

void IRRIGATION_CTRL_CancelProgram(void) { IRRIGATION_CTRL_Stop(); }

uint8_t IRRIGATION_CTRL_GetActiveProgram(void) {
  return PARCEL_SCHED_GetActiveProgram();
}

program_state_t IRRIGATION_CTRL_GetProgramState(void) {
  return IRRIGATION_RUNTIME_GetProgramState();
}

void IRRIGATION_CTRL_Stop(void) {
  IRRIGATION_RUN_Stop(&ctrl);
  IRRIGATION_CTRL_ChangeState(CTRL_STATE_IDLE);
  IRRIGATION_CTRL_ClearRuntimeBackup();
}

void IRRIGATION_CTRL_Pause(void) { IRRIGATION_RUN_Pause(&ctrl); }

void IRRIGATION_CTRL_Resume(void) { IRRIGATION_RUN_Resume(&ctrl); }

uint8_t IRRIGATION_CTRL_IsRunning(void) { return ctrl.is_running; }

void IRRIGATION_CTRL_Update(void) {
  irrigation_schedule_context_t schedule_context = {0};

  if (ctrl.is_running == 0U || ctrl.is_paused != 0U) {
    return;
  }

  switch (IRRIGATION_RUNTIME_GetProgramState()) {
  case PROGRAM_STATE_PRE_FLUSH:
  case PROGRAM_STATE_POST_FLUSH:
    IRRIGATION_CTRL_ServiceFlushPhase();
    break;
  case PROGRAM_STATE_VALVE_ACTIVE:
    IRRIGATION_CTRL_ServiceValveActive();
    break;
  case PROGRAM_STATE_WAITING:
    IRRIGATION_CTRL_ServiceWaiting();
    break;
  case PROGRAM_STATE_NEXT_VALVE:
    if (PARCEL_SCHED_Advance(g_programs) == 0U) {
      IRRIGATION_CTRL_FinishRun();
      break;
    }
    (void)IRRIGATION_CTRL_StartCurrentValveCycle();
    break;
  case PROGRAM_STATE_ERROR:
    IRRIGATION_CTRL_EmergencyStop();
    break;
  case PROGRAM_STATE_IDLE:
  default:
    break;
  }

  if (PARCEL_SCHED_GetActiveProgram() != 0U && ctrl.is_running != 0U) {
    irrigation_program_t *program =
        &g_programs[PARCEL_SCHED_GetActiveProgram() - 1U];
    IRRIGATION_TRIGGER_ReadContext(&schedule_context);
    if (IRRIGATION_TRIGGER_IsPastEndWindow(&schedule_context,
                                           program->end_hhmm) != 0U) {
      IRRIGATION_CTRL_SetError(ERR_TIMEOUT);
    }
  }
}

control_state_t IRRIGATION_CTRL_GetState(void) { return ctrl.state; }

const char *IRRIGATION_CTRL_GetStateName(control_state_t state) {
  switch (state) {
  case CTRL_STATE_IDLE:
    return "IDLE";
  case CTRL_STATE_PARCEL_WATERING:
    return "VALVE";
  case CTRL_STATE_WAITING:
    return "WAIT";
  case CTRL_STATE_ERROR:
    return "ERROR";
  case CTRL_STATE_EMERGENCY_STOP:
    return "STOP";
  case CTRL_STATE_CHECKING_SENSORS:
    return "CHECK";
  case CTRL_STATE_MIXING:
    return "MIX";
  case CTRL_STATE_SETTLING:
    return "SETTLE";
  case CTRL_STATE_PRE_FLUSH:
    return "PREFLUSH";
  case CTRL_STATE_POST_FLUSH:
    return "POSTFLUSH";
  case CTRL_STATE_PH_ADJUSTING:
    return "PH";
  case CTRL_STATE_EC_ADJUSTING:
    return "EC";
  default:
    return "RUN";
  }
}

void IRRIGATION_CTRL_ReadSensors(void) {
  ctrl.ph_data.ph_value = PH_GetValue();
  ctrl.ph_data.status = PH_GetStatus();
  ctrl.ec_data.ec_value = EC_GetValue();
  ctrl.ec_data.status = EC_GetStatus();
}

float IRRIGATION_CTRL_GetPH(void) { return PH_GetValue(); }

float IRRIGATION_CTRL_GetEC(void) { return EC_GetValue(); }

uint8_t IRRIGATION_CTRL_IsPHValid(void) { return (PH_GetStatus() == SENSOR_OK) ? 1U : 0U; }

uint8_t IRRIGATION_CTRL_IsECValid(void) { return (EC_GetStatus() == SENSOR_OK) ? 1U : 0U; }

uint8_t IRRIGATION_CTRL_CheckPH(void) { return IRRIGATION_CTRL_IsPHInRange(); }

void IRRIGATION_CTRL_AdjustPH(void) { IRRIGATION_CTRL_ServiceDosing(); }

uint8_t IRRIGATION_CTRL_IsPHInRange(void) {
  return (fabsf(ctrl.ph_data.ph_value - ctrl.ph_params.target) <=
          ctrl.ph_params.hysteresis)
             ? 1U
             : 0U;
}

uint8_t IRRIGATION_CTRL_CheckEC(void) { return IRRIGATION_CTRL_IsECInRange(); }

void IRRIGATION_CTRL_AdjustEC(void) { IRRIGATION_CTRL_ServiceDosing(); }

uint8_t IRRIGATION_CTRL_IsECInRange(void) {
  return (fabsf(ctrl.ec_data.ec_value - ctrl.ec_params.target) <=
          ctrl.ec_params.hysteresis)
             ? 1U
             : 0U;
}

void IRRIGATION_CTRL_StartParcel(uint8_t parcel_id) {
  IRRIGATION_CTRL_ClearQueue();
  IRRIGATION_CTRL_AddToQueue(parcel_id);
  IRRIGATION_CTRL_Start();
}

void IRRIGATION_CTRL_StopParcel(void) { IRRIGATION_CTRL_Stop(); }

void IRRIGATION_CTRL_NextParcel(void) {
  IRRIGATION_RUNTIME_SetProgramState(PROGRAM_STATE_NEXT_VALVE);
}

uint8_t IRRIGATION_CTRL_IsParcelComplete(void) {
  return (IRRIGATION_CTRL_GetRemainingTime() == 0U) ? 1U : 0U;
}

uint8_t IRRIGATION_CTRL_GetCurrentParcelId(void) {
  return ctrl.current_parcel.parcel_id;
}

uint32_t IRRIGATION_CTRL_GetRemainingTime(void) {
  return IRRIGATION_RUN_GetRemainingTime(&ctrl, g_programs,
                                         IRRIGATION_RUNTIME_GetProgramState());
}

void IRRIGATION_CTRL_AddToQueue(uint8_t parcel_id) {
  (void)PARCEL_SCHED_AddToQueue(parcel_id);
}

void IRRIGATION_CTRL_ClearQueue(void) {
  PARCEL_SCHED_ClearQueue();
}

void IRRIGATION_CTRL_StartMixing(void) {
  DOSING_CTRL_StartMixing(&ctrl.ph_params, &ctrl.ec_params, CTRL_STATE_PARCEL_WATERING);
  IRRIGATION_CTRL_ChangeState(DOSING_CTRL_GetRecommendedState());
}

void IRRIGATION_CTRL_StopMixing(void) {
  DOSING_CTRL_StopMixing();
}

uint8_t IRRIGATION_CTRL_IsMixingComplete(void) {
  return DOSING_CTRL_IsMixingComplete(&ctrl.ph_params, &ctrl.ec_params);
}

void IRRIGATION_CTRL_StartSettling(void) {
  DOSING_CTRL_StartSettling(CTRL_STATE_PARCEL_WATERING);
  IRRIGATION_CTRL_ChangeState(DOSING_CTRL_GetRecommendedState());
}

uint8_t IRRIGATION_CTRL_IsSettlingComplete(void) {
  return DOSING_CTRL_IsSettlingComplete(&ctrl.ph_params, &ctrl.ec_params);
}

void IRRIGATION_CTRL_SetError(error_code_t error) {
  IRRIGATION_ALARM_RaiseError(&ctrl, error, IRRIGATION_CTRL_ChangeState,
                              IRRIGATION_CTRL_OnError);
}

void IRRIGATION_CTRL_ClearError(error_code_t error) {
  IRRIGATION_ALARM_Clear(&ctrl, error);
}

void IRRIGATION_CTRL_ClearAllErrors(void) {
  if (FAULT_MGR_HasActiveFault() == 0U) {
    IRRIGATION_ALARM_ClearAll(&ctrl);
    return;
  }

  (void)IRRIGATION_CTRL_ResetAlarm();
}

error_code_t IRRIGATION_CTRL_GetLastError(void) { return FAULT_MGR_GetLastError(); }

uint8_t IRRIGATION_CTRL_HasErrors(void) { return FAULT_MGR_HasActiveFault(); }

const char *IRRIGATION_CTRL_GetErrorString(error_code_t error) {
  return FAULT_MGR_GetErrorString(error);
}

void IRRIGATION_CTRL_EmergencyStop(void) {
  IRRIGATION_ALARM_EnterEmergencyStop(&ctrl, ERR_TIMEOUT,
                                      IRRIGATION_CTRL_ChangeState);
}

uint8_t IRRIGATION_CTRL_AcknowledgeAlarm(void) {
  return IRRIGATION_ALARM_Acknowledge(&ctrl);
}

uint8_t IRRIGATION_CTRL_ResetAlarm(void) {
  return IRRIGATION_ALARM_Reset(&ctrl, IRRIGATION_CTRL_Stop,
                                IRRIGATION_CTRL_RunSafetyChecks);
}

uint8_t IRRIGATION_CTRL_CanResetAlarm(void) { return FAULT_MGR_CanReset(); }

uint8_t IRRIGATION_CTRL_IsAlarmAcknowledged(void) {
  return FAULT_MGR_IsAcknowledged();
}

const char *IRRIGATION_CTRL_GetAlarmDetailText(void) {
  return FAULT_MGR_GetDetailText();
}

const char *IRRIGATION_CTRL_GetAlarmActionText(void) {
  return FAULT_MGR_GetActionText();
}

uint32_t IRRIGATION_CTRL_GetTotalWateringTime(void) {
  return ctrl.total_watering_time;
}

uint32_t IRRIGATION_CTRL_GetTotalParcelCount(void) {
  return ctrl.total_parcel_count;
}

uint32_t IRRIGATION_CTRL_GetCurrentRunTime(void) {
  if (ctrl.current_parcel.parcel_id == 0U) {
    return 0U;
  }
  return (HAL_GetTick() - ctrl.current_parcel.start_time) / 1000U;
}

void IRRIGATION_CTRL_ResetStatistics(void) {
  ctrl.total_watering_time = 0U;
  ctrl.total_parcel_count = 0U;
}

void IRRIGATION_CTRL_SetSchedule(uint8_t slot, irrigation_schedule_t *sched) {
  irrigation_program_t program = {0};

  if (slot >= IRRIGATION_PROGRAM_COUNT || sched == NULL) {
    return;
  }

  g_schedules[slot] = *sched;
  IRRIGATION_CTRL_GetProgram(slot + 1U, &program);
  program.enabled = sched->enabled;
  program.start_hhmm = (uint16_t)((sched->hour * 100U) + sched->minute);
  program.valve_mask = (sched->parcel_id == 0U) ? 0U : (uint8_t)(1U << (sched->parcel_id - 1U));
  program.days_mask = sched->days_of_week;
  if (program.irrigation_min == 0U) {
    program.irrigation_min = 5U;
  }
  if (sched->parcel_id >= 1U && sched->parcel_id <= IRRIGATION_PROGRAM_VALVE_COUNT) {
    uint32_t duration_sec = PARCELS_GetDuration(sched->parcel_id);
    if (duration_sec != 0U) {
      uint32_t duration_min = (duration_sec + 59U) / 60U;
      if (duration_min > 255U) {
        duration_min = 255U;
      }
      program.irrigation_min = (uint16_t)duration_min;
    }
  }
  if (program.repeat_count == 0U) {
    program.repeat_count = 1U;
  }
  if (program.end_hhmm == 0U) {
    program.end_hhmm = (uint16_t)(program.start_hhmm + 100U);
  }
  IRRIGATION_CTRL_SetProgram(slot + 1U, &program);
}

void IRRIGATION_CTRL_GetSchedule(uint8_t slot, irrigation_schedule_t *sched) {
  if (slot >= IRRIGATION_PROGRAM_COUNT || sched == NULL) {
    return;
  }

  if (g_schedules[slot].enabled == 0U && g_programs[slot].enabled != 0U) {
    g_schedules[slot].enabled = g_programs[slot].enabled;
    g_schedules[slot].hour = (uint8_t)(g_programs[slot].start_hhmm / 100U);
    g_schedules[slot].minute = (uint8_t)(g_programs[slot].start_hhmm % 100U);
    g_schedules[slot].days_of_week = g_programs[slot].days_mask;
    for (uint8_t i = 0U; i < IRRIGATION_PROGRAM_VALVE_COUNT; i++) {
      if ((g_programs[slot].valve_mask & (1U << i)) != 0U) {
        g_schedules[slot].parcel_id = i + 1U;
        break;
      }
    }
  }

  *sched = g_schedules[slot];
}

void IRRIGATION_CTRL_CheckSchedules(void) {
  irrigation_schedule_context_t schedule_context = {0};
  irrigation_program_t *program;
  uint8_t program_id = 0U;

  if (ctrl.is_running != 0U) {
    return;
  }

  if (g_auto_mode != AUTO_MODE_SCHEDULED && g_auto_mode != AUTO_MODE_FULL_AUTO) {
    return;
  }

  IRRIGATION_TRIGGER_ReadContext(&schedule_context);
  if (schedule_context.rtc_ready == 0U) {
    return;
  }

  if (IRRIGATION_TRIGGER_SelectProgram(&schedule_context, g_programs,
                                       IRRIGATION_PROGRAM_COUNT, &program_id) ==
      0U) {
    return;
  }

  program = &g_programs[program_id - 1U];
  IRRIGATION_CTRL_StartProgram(program_id);

  /* Mark the slot as consumed only after the program really enters a run.
   * Otherwise a transient pre-start failure blocks all retries for that day. */
  if (ctrl.is_running == 0U || PARCEL_SCHED_GetActiveProgram() != program_id) {
    return;
  }

  program->last_run_day = schedule_context.day_stamp;
  program->last_run_minute = schedule_context.minute_of_day;
  IRRIGATION_PERSIST_QueueProgram(program_id, program);
}

void IRRIGATION_CTRL_GetProgram(uint8_t program_id, irrigation_program_t *program) {
  if (program_id == 0U || program_id > IRRIGATION_PROGRAM_COUNT || program == NULL) {
    return;
  }

  *program = g_programs[program_id - 1U];
}

void IRRIGATION_CTRL_SetProgram(uint8_t program_id,
                                const irrigation_program_t *program) {
  irrigation_program_t record;
  irrigation_program_t *stored_program;

  if (program_id == 0U || program_id > IRRIGATION_PROGRAM_COUNT || program == NULL) {
    return;
  }

  stored_program = &g_programs[program_id - 1U];
  record = *program;

  if (record.enabled != stored_program->enabled ||
      record.start_hhmm != stored_program->start_hhmm ||
      record.end_hhmm != stored_program->end_hhmm ||
      record.valve_mask != stored_program->valve_mask ||
      record.irrigation_min != stored_program->irrigation_min ||
      record.wait_min != stored_program->wait_min ||
      record.repeat_count != stored_program->repeat_count ||
      record.days_mask != stored_program->days_mask ||
      record.pre_flush_sec != stored_program->pre_flush_sec ||
      record.post_flush_sec != stored_program->post_flush_sec) {
    record.last_run_day = 0U;
    record.last_run_minute = 0xFFFFU;
  }

  *stored_program = record;
  IRRIGATION_CTRL_NormalizeProgram(stored_program);
  stored_program->crc =
      EEPROM_CalculateCRC(stored_program,
                          sizeof(irrigation_program_t) - 2U);
  IRRIGATION_PERSIST_QueueProgram(program_id, stored_program);
  IRRIGATION_PERSIST_Service();
}

void IRRIGATION_CTRL_LoadPrograms(void) {
  (void)IRRIGATION_PERSIST_LoadPrograms(g_programs, IRRIGATION_PROGRAM_COUNT);
  for (uint8_t i = 0U; i < IRRIGATION_PROGRAM_COUNT; i++) {
    IRRIGATION_CTRL_NormalizeProgram(&g_programs[i]);
  }
}

void IRRIGATION_CTRL_LoadRuntimeBackup(void) {
  irrigation_program_t *program;
  uint32_t duration_sec;
  program_state_t restored_state;

  if (IRRIGATION_RUNTIME_LoadBackup(&g_runtime_backup) != EEPROM_OK) {
    memset(&g_runtime_backup, 0, sizeof(g_runtime_backup));
    return;
  }

  if (g_runtime_backup.valid == 0U) {
    memset(&g_runtime_backup, 0, sizeof(g_runtime_backup));
    return;
  }

  if (g_runtime_backup.active_program_id == 0U ||
      g_runtime_backup.active_program_id > IRRIGATION_PROGRAM_COUNT) {
    IRRIGATION_CTRL_ClearRuntimeBackup();
    return;
  }

  program = &g_programs[g_runtime_backup.active_program_id - 1U];
  if (program->enabled == 0U ||
      PARCEL_SCHED_BuildSequenceFromMask(program->valve_mask) == 0U) {
    IRRIGATION_CTRL_ClearRuntimeBackup();
    return;
  }

  restored_state = (program_state_t)g_runtime_backup.program_state;
  if (IRRIGATION_CTRL_IsRestorableState(restored_state) == 0U) {
    IRRIGATION_CTRL_ClearRuntimeBackup();
    return;
  }

  PARCEL_SCHED_SetManualSequence(0U);
  PARCEL_SCHED_SetActiveProgram(g_runtime_backup.active_program_id);
  PARCEL_SCHED_SetActiveValveIndex(g_runtime_backup.active_valve_index);
  PARCEL_SCHED_SetRepeatIndex(g_runtime_backup.repeat_index);
  if (g_runtime_backup.error_code != (uint8_t)ERR_NONE) {
    IRRIGATION_CTRL_AbortRuntimeRestore(
        (error_code_t)g_runtime_backup.error_code);
    return;
  }
  FAULT_MGR_ClearAll();
  IRRIGATION_ALARM_Sync(&ctrl);
  ctrl.ph_params.target = (float)g_runtime_backup.ph_target_x100 / 100.0f;
  ctrl.ec_params.target = (float)g_runtime_backup.ec_target_x100 / 100.0f;

  if (PARCEL_SCHED_GetActiveValveIndex() >= PARCEL_SCHED_GetActiveValveCount()) {
    IRRIGATION_CTRL_ClearRuntimeBackup();
    return;
  }

  if ((restored_state == PROGRAM_STATE_PRE_FLUSH ||
       restored_state == PROGRAM_STATE_VALVE_ACTIVE ||
       restored_state == PROGRAM_STATE_POST_FLUSH) &&
      g_runtime_backup.active_valve_id != 0U &&
      g_runtime_backup.active_valve_id != PARCEL_SCHED_GetCurrentValve()) {
    IRRIGATION_CTRL_ClearRuntimeBackup();
    return;
  }

  if (g_runtime_backup.remaining_sec == 0U) {
    if (restored_state == PROGRAM_STATE_PRE_FLUSH) {
      restored_state = PROGRAM_STATE_VALVE_ACTIVE;
      g_runtime_backup.remaining_sec =
          (uint16_t)IRRIGATION_RUN_GetCurrentPhaseDurationSec(g_programs,
                                                              restored_state);
    } else if (restored_state == PROGRAM_STATE_VALVE_ACTIVE &&
               IRRIGATION_RUN_GetCurrentPhaseDurationSec(
                   g_programs, PROGRAM_STATE_POST_FLUSH) > 0U) {
      restored_state = PROGRAM_STATE_POST_FLUSH;
      g_runtime_backup.remaining_sec =
          (uint16_t)IRRIGATION_RUN_GetCurrentPhaseDurationSec(g_programs,
                                                              restored_state);
    } else if (restored_state == PROGRAM_STATE_VALVE_ACTIVE ||
               restored_state == PROGRAM_STATE_POST_FLUSH) {
      restored_state = PROGRAM_STATE_NEXT_VALVE;
    }
  }

  if (restored_state == PROGRAM_STATE_NEXT_VALVE) {
    if (PARCEL_SCHED_Advance(g_programs) == 0U) {
      IRRIGATION_CTRL_ClearRuntimeBackup();
      return;
    }
    restored_state = IRRIGATION_CTRL_GetInitialValvePhase();
    g_runtime_backup.remaining_sec =
        (uint16_t)IRRIGATION_RUN_GetCurrentPhaseDurationSec(g_programs,
                                                            restored_state);
  }

  IRRIGATION_RUN_Begin(&ctrl);
  IRRIGATION_RUNTIME_SetProgramState(restored_state);
  if (IRRIGATION_RUNTIME_GetProgramState() == PROGRAM_STATE_WAITING) {
    uint32_t wait_duration_sec =
        IRRIGATION_RUN_GetCurrentWaitDurationSec(g_programs);
    if (g_runtime_backup.remaining_sec > wait_duration_sec) {
      g_runtime_backup.remaining_sec = (uint16_t)wait_duration_sec;
    }
    PARCEL_SCHED_SetWaitStartTick(
        HAL_GetTick() - ((wait_duration_sec - g_runtime_backup.remaining_sec) * 1000UL));
    IRRIGATION_CTRL_ChangeState(CTRL_STATE_WAITING);
    IRRIGATION_CTRL_PersistRuntimeBackup();
    return;
  }

  duration_sec = IRRIGATION_RUN_GetCurrentPhaseDurationSec(g_programs,
                                                           restored_state);
  IRRIGATION_CTRL_ChangeState(
      IRRIGATION_CTRL_GetControlStateForProgramPhase(restored_state));
  if (IRRIGATION_RUN_ActivateCurrentValve(&ctrl, g_programs,
                                          restored_state) == 0U) {
    IRRIGATION_CTRL_SetError(ERR_VALVE_STUCK);
    return;
  }
  IRRIGATION_RUN_RestoreCurrentValveTiming(&ctrl, duration_sec,
                                           g_runtime_backup.remaining_sec);
  IRRIGATION_CTRL_PersistRuntimeBackup();
}

void IRRIGATION_CTRL_MaintenanceTask(void) {
  IRRIGATION_PERSIST_Service();

  IRRIGATION_RUNTIME_ServiceSnapshot(ctrl.is_running,
                                     SYSTEM_RUNTIME_SNAPSHOT_PERIOD_MS);

  if (IRRIGATION_RUNTIME_NeedsPersist() != 0U) {
    IRRIGATION_CTRL_PersistRuntimeBackup();
  }
}

uint8_t IRRIGATION_CTRL_RunSafetyChecks(void) {
  irrigation_safety_result_t safety_result = {0};

  IRRIGATION_SAFETY_Evaluate(ctrl.current_parcel.valve_id, &safety_result);
  FAULT_MGR_SetValveErrorMask(safety_result.valve_error_mask);

  if (safety_result.action == IRRIGATION_SAFETY_ACTION_EMERGENCY_STOP) {
    IRRIGATION_CTRL_EmergencyStop();
    return 0U;
  }

  if (safety_result.action == IRRIGATION_SAFETY_ACTION_ERROR) {
    IRRIGATION_CTRL_SetError(safety_result.error);
    return 0U;
  }

  return safety_result.healthy;
}

const char *IRRIGATION_CTRL_GetActiveAlarmText(void) {
  return FAULT_MGR_GetAlarmText();
}

void IRRIGATION_CTRL_GetRuntimeBackup(irrigation_runtime_backup_t *backup) {
  if (backup == NULL) {
    return;
  }

  IRRIGATION_CTRL_UpdateRuntimeBackup();
  *backup = g_runtime_backup;
}

void IRRIGATION_CTRL_OnStateChange(control_state_t old_state,
                                   control_state_t new_state) {
  (void)old_state;
  (void)new_state;
}

void IRRIGATION_CTRL_OnPHAdjusted(float new_ph) { (void)new_ph; }

void IRRIGATION_CTRL_OnECAdjusted(float new_ec) { (void)new_ec; }

void IRRIGATION_CTRL_OnParcelComplete(uint8_t parcel_id) { (void)parcel_id; }

void IRRIGATION_CTRL_OnError(error_code_t error) { (void)error; }

static void IRRIGATION_CTRL_ApplySystemRecord(void) {
  (void)IRRIGATION_PERSIST_LoadSystemRecord(&g_system_record);
  g_system_record.system_flags = 0U;

  if (g_system_record.ph_target > 0.0f) {
    ctrl.ph_params.target = g_system_record.ph_target;
    ctrl.ph_params.min_limit = g_system_record.ph_min;
    ctrl.ph_params.max_limit = g_system_record.ph_max;
  }

  if (g_system_record.ec_target > 0.0f) {
    ctrl.ec_params.target = g_system_record.ec_target;
    ctrl.ec_params.min_limit = g_system_record.ec_min;
    ctrl.ec_params.max_limit = g_system_record.ec_max;
  }

  ctrl.ph_params.feedback_delay_ms = g_system_record.ph_feedback_delay_ms;
  ctrl.ph_params.response_gain_percent = g_system_record.ph_response_gain_percent;
  ctrl.ph_params.max_correction_cycles = g_system_record.ph_max_correction_cycles;
  ctrl.ec_params.feedback_delay_ms = g_system_record.ec_feedback_delay_ms;
  ctrl.ec_params.response_gain_percent = g_system_record.ec_response_gain_percent;
  ctrl.ec_params.max_correction_cycles = g_system_record.ec_max_correction_cycles;
  g_auto_mode = IRRIGATION_CTRL_GetStoredAutoMode();
}

static void IRRIGATION_CTRL_SetDefaults(void) {
  ctrl.state = CTRL_STATE_IDLE;
  ctrl.ph_params.target = 6.50f;
  ctrl.ph_params.min_limit = 5.50f;
  ctrl.ph_params.max_limit = 7.50f;
  ctrl.ph_params.hysteresis = IRRIGATION_PH_HYSTERESIS;
  ctrl.ph_params.dose_time_ms = 500U;
  ctrl.ph_params.wait_time_ms = IRRIGATION_MIXING_TIME * 1000U;
  ctrl.ph_params.auto_adjust_enabled = 1U;
  ctrl.ph_params.pwm_duty_percent = 35U;
  ctrl.ph_params.feedback_delay_ms = IRRIGATION_SETTLING_TIME * 1000U;
  ctrl.ph_params.response_gain_percent = 100U;
  ctrl.ph_params.max_correction_cycles = 3U;
  ctrl.ph_params.acid_valve_id = DOSING_VALVE_ACID_ID;
  ctrl.ph_params.base_valve_id = 0U;
  ctrl.ph_params.fertilizer_select = 0U;

  ctrl.ec_params.target = 1.80f;
  ctrl.ec_params.min_limit = 1.00f;
  ctrl.ec_params.max_limit = 2.50f;
  ctrl.ec_params.hysteresis = IRRIGATION_EC_HYSTERESIS;
  ctrl.ec_params.dose_time_ms = 500U;
  ctrl.ec_params.wait_time_ms = IRRIGATION_MIXING_TIME * 1000U;
  ctrl.ec_params.auto_adjust_enabled = 1U;
  ctrl.ec_params.pwm_duty_percent = 40U;
  ctrl.ec_params.feedback_delay_ms = IRRIGATION_SETTLING_TIME * 1000U;
  ctrl.ec_params.response_gain_percent = 100U;
  ctrl.ec_params.max_correction_cycles = 3U;
  ctrl.ec_params.fertilizer_valve_id = DOSING_VALVE_FERT_A_ID;
  ctrl.ec_params.fertilizer_select = 0U;
  ctrl.ec_params.recipe_ratio[0] = 25U;
  ctrl.ec_params.recipe_ratio[1] = 25U;
  ctrl.ec_params.recipe_ratio[2] = 25U;
  ctrl.ec_params.recipe_ratio[3] = 25U;

  VALVES_ConfigureDosingChannel(DOSING_VALVE_ACID_ID,
                                DOSING_ACID_PWM_DEFAULT_FREQ_HZ, 5U, 95U, 8U);
  VALVES_ConfigureDosingChannel(DOSING_VALVE_FERT_A_ID,
                                DOSING_FERT_PWM_DEFAULT_FREQ_HZ, 5U, 95U, 5U);
  VALVES_ConfigureDosingChannel(DOSING_VALVE_FERT_B_ID,
                                DOSING_FERT_PWM_DEFAULT_FREQ_HZ, 5U, 95U, 5U);
  VALVES_ConfigureDosingChannel(DOSING_VALVE_FERT_C_ID,
                                DOSING_FERT_PWM_DEFAULT_FREQ_HZ, 5U, 95U, 5U);
  VALVES_ConfigureDosingChannel(DOSING_VALVE_FERT_D_ID,
                                DOSING_FERT_PWM_DEFAULT_FREQ_HZ, 5U, 95U, 5U);
}

static auto_mode_t IRRIGATION_CTRL_GetStoredAutoMode(void) {
  if (g_system_record.reserved[1] == EEPROM_SYSTEM_MODE_MARKER &&
      g_system_record.reserved[0] <= (uint8_t)AUTO_MODE_SCHEDULED) {
    return (auto_mode_t)g_system_record.reserved[0];
  }

  return (g_system_record.auto_mode_enabled != 0U) ? AUTO_MODE_SCHEDULED
                                                   : AUTO_MODE_OFF;
}

static void IRRIGATION_CTRL_StoreAutoMode(auto_mode_t mode) {
  g_system_record.auto_mode_enabled = (mode != AUTO_MODE_OFF) ? 1U : 0U;
  g_system_record.reserved[0] = (uint8_t)mode;
  g_system_record.reserved[1] = EEPROM_SYSTEM_MODE_MARKER;
}

static void IRRIGATION_CTRL_PersistRuntimeBackup(void) {
  if (IRRIGATION_RUNTIME_NeedsPersist() == 0U) {
    return;
  }

  IRRIGATION_CTRL_UpdateRuntimeBackup();
  (void)IRRIGATION_RUNTIME_Persist(ctrl.is_running, &g_runtime_backup);
}

static void IRRIGATION_CTRL_ClearRuntimeBackup(void) {
  memset(&g_runtime_backup, 0, sizeof(g_runtime_backup));
  IRRIGATION_RUNTIME_RequestPersist();
  (void)IRRIGATION_RUNTIME_Persist(0U, &g_runtime_backup);
}

static void IRRIGATION_CTRL_ChangeState(control_state_t new_state) {
  control_state_t old_state = ctrl.state;

  ctrl.state = new_state;
  ctrl.timers.state_entry_time = HAL_GetTick();
  GUI_UpdateIrrigationStatus(ctrl.is_running, ctrl.current_parcel.parcel_id);
  if (old_state != new_state) {
    IRRIGATION_CTRL_OnStateChange(old_state, new_state);
  }
}

static void IRRIGATION_CTRL_FinishRun(void) {
  IRRIGATION_CTRL_Stop();
  FAULT_MGR_SetAlarmText("PROGRAM DONE");
}

static uint8_t IRRIGATION_CTRL_StartPreparedRun(void) {
  IRRIGATION_RUN_Begin(&ctrl);
  FAULT_MGR_ClearAll();
  VALVES_ClearErrors();
  IRRIGATION_ALARM_Sync(&ctrl);
  return IRRIGATION_CTRL_StartCurrentValveCycle();
}

static uint8_t IRRIGATION_CTRL_StartCurrentValveCycle(void) {
  program_state_t phase = IRRIGATION_CTRL_GetInitialValvePhase();

  DOSING_CTRL_Reset();
  IRRIGATION_CTRL_ChangeState(
      IRRIGATION_CTRL_GetControlStateForProgramPhase(phase));
  if (IRRIGATION_RUN_ActivateCurrentValve(&ctrl, g_programs, phase) == 0U) {
    IRRIGATION_CTRL_SetError(ERR_VALVE_STUCK);
    return 0U;
  }
  IRRIGATION_CTRL_PersistRuntimeBackup();
  return 1U;
}

static uint8_t IRRIGATION_CTRL_IsRestorableState(program_state_t state) {
  switch (state) {
  case PROGRAM_STATE_PRE_FLUSH:
  case PROGRAM_STATE_VALVE_ACTIVE:
  case PROGRAM_STATE_POST_FLUSH:
  case PROGRAM_STATE_WAITING:
  case PROGRAM_STATE_NEXT_VALVE:
    return 1U;
  case PROGRAM_STATE_IDLE:
  case PROGRAM_STATE_ERROR:
  default:
    return 0U;
  }
}

static void IRRIGATION_CTRL_AbortRuntimeRestore(error_code_t error) {
  if (error == ERR_NONE) {
    error = ERR_TIMEOUT;
  }

  IRRIGATION_ALARM_RaiseError(&ctrl, error, IRRIGATION_CTRL_ChangeState,
                              IRRIGATION_CTRL_OnError);
  IRRIGATION_CTRL_ClearRuntimeBackup();
}

static program_state_t IRRIGATION_CTRL_GetInitialValvePhase(void) {
  if (IRRIGATION_RUN_GetCurrentPhaseDurationSec(g_programs,
                                                PROGRAM_STATE_PRE_FLUSH) > 0U) {
    return PROGRAM_STATE_PRE_FLUSH;
  }

  return PROGRAM_STATE_VALVE_ACTIVE;
}

static control_state_t IRRIGATION_CTRL_GetControlStateForProgramPhase(
    program_state_t phase) {
  if (phase == PROGRAM_STATE_PRE_FLUSH) {
    return CTRL_STATE_PRE_FLUSH;
  }

  if (phase == PROGRAM_STATE_POST_FLUSH) {
    return CTRL_STATE_POST_FLUSH;
  }

  return CTRL_STATE_PARCEL_WATERING;
}

static void IRRIGATION_CTRL_HandleCompletedValveStep(
    irrigation_run_step_t run_step) {
  if (run_step == IRRIGATION_RUN_STEP_WAITING) {
    IRRIGATION_RUNTIME_SetProgramState(PROGRAM_STATE_WAITING);
    IRRIGATION_CTRL_ChangeState(CTRL_STATE_WAITING);
    IRRIGATION_CTRL_PersistRuntimeBackup();
    return;
  }

  if (run_step == IRRIGATION_RUN_STEP_NEXT_VALVE) {
    IRRIGATION_RUNTIME_SetProgramState(PROGRAM_STATE_NEXT_VALVE);
  }
}

static void IRRIGATION_CTRL_ServiceFlushPhase(void) {
  program_state_t phase = IRRIGATION_RUNTIME_GetProgramState();
  irrigation_run_step_t run_step;

  run_step = IRRIGATION_RUN_ServiceValveTiming(&ctrl, g_programs, phase,
                                               IRRIGATION_CTRL_OnParcelComplete);
  if (run_step == IRRIGATION_RUN_STEP_NONE) {
    return;
  }

  if (phase == PROGRAM_STATE_PRE_FLUSH &&
      run_step == IRRIGATION_RUN_STEP_PHASE_DONE) {
    IRRIGATION_RUN_StartPhase(&ctrl, g_programs, PROGRAM_STATE_VALVE_ACTIVE);
    IRRIGATION_CTRL_ChangeState(CTRL_STATE_PARCEL_WATERING);
    IRRIGATION_CTRL_PersistRuntimeBackup();
    return;
  }

  IRRIGATION_CTRL_HandleCompletedValveStep(run_step);
}

static void IRRIGATION_CTRL_ServiceValveActive(void) {
  irrigation_run_step_t run_step;

  IRRIGATION_CTRL_ServiceDosing();

  run_step = IRRIGATION_RUN_ServiceValveTiming(&ctrl, g_programs,
                                               PROGRAM_STATE_VALVE_ACTIVE,
                                               IRRIGATION_CTRL_OnParcelComplete);
  if (run_step == IRRIGATION_RUN_STEP_NONE) {
    return;
  }

  if (run_step == IRRIGATION_RUN_STEP_PHASE_DONE) {
    DOSING_CTRL_Reset();
    IRRIGATION_RUN_StartPhase(&ctrl, g_programs, PROGRAM_STATE_POST_FLUSH);
    IRRIGATION_CTRL_ChangeState(CTRL_STATE_POST_FLUSH);
    IRRIGATION_CTRL_PersistRuntimeBackup();
    return;
  }

  IRRIGATION_CTRL_HandleCompletedValveStep(run_step);
}

static void IRRIGATION_CTRL_ServiceWaiting(void) {
  if (IRRIGATION_RUN_ServiceWaiting(g_programs) == 0U) {
    return;
  }

  IRRIGATION_RUNTIME_SetProgramState(PROGRAM_STATE_NEXT_VALVE);
}

static void IRRIGATION_CTRL_ServiceDosing(void) {
  dosing_controller_status_t dosing_status;

  IRRIGATION_CTRL_ReadSensors();
  DOSING_CTRL_Update(&ctrl.ph_params, &ctrl.ec_params, &ctrl.ph_data, &ctrl.ec_data,
                     CTRL_STATE_PARCEL_WATERING);
  DOSING_CTRL_GetStatus(&dosing_status);

  if (dosing_status.last_error != ERR_NONE) {
    IRRIGATION_CTRL_SetError(dosing_status.last_error);
    return;
  }

  if (dosing_status.recommended_state != ctrl.state) {
    IRRIGATION_CTRL_ChangeState(dosing_status.recommended_state);
  }
}

static void IRRIGATION_CTRL_UpdateRuntimeBackup(void) {
  IRRIGATION_ALARM_Sync(&ctrl);
  memset(&g_runtime_backup, 0, sizeof(g_runtime_backup));
  g_runtime_backup.valid = (ctrl.is_running != 0U) ? 1U : 0U;
  g_runtime_backup.active_program_id = PARCEL_SCHED_GetActiveProgram();
  g_runtime_backup.active_valve_index = PARCEL_SCHED_GetActiveValveIndex();
  g_runtime_backup.repeat_index = PARCEL_SCHED_GetRepeatIndex();
  g_runtime_backup.program_state =
      (uint8_t)IRRIGATION_RUNTIME_GetProgramState();
  g_runtime_backup.active_valve_id = ctrl.current_parcel.valve_id;
  g_runtime_backup.remaining_sec = (uint16_t)IRRIGATION_CTRL_GetRemainingTime();
  g_runtime_backup.ec_target_x100 = (int16_t)(ctrl.ec_params.target * 100.0f);
  g_runtime_backup.ph_target_x100 = (int16_t)(ctrl.ph_params.target * 100.0f);
  g_runtime_backup.error_code = (uint8_t)FAULT_MGR_GetLastError();
  g_runtime_backup.crc =
      EEPROM_CalculateCRC(&g_runtime_backup, sizeof(g_runtime_backup) - 2U);
}

static void IRRIGATION_CTRL_NormalizeProgram(irrigation_program_t *program) {
  if (program == NULL) {
    return;
  }

  if (program->repeat_count == 0U) {
    program->repeat_count = 1U;
  }

  if (program->irrigation_min == 0U) {
    for (uint8_t i = 0U; i < IRRIGATION_PROGRAM_VALVE_COUNT; i++) {
      if ((program->valve_mask & (1U << i)) != 0U) {
        uint32_t duration_sec = PARCELS_GetDuration((uint8_t)(i + 1U));
        if (duration_sec != 0U) {
          uint32_t duration_min = (duration_sec + 59U) / 60U;
          program->irrigation_min =
              (uint16_t)((duration_min > 999U) ? 999U : duration_min);
          break;
        }
      }
    }
  }

  if (program->irrigation_min == 0U) {
    program->irrigation_min = 1U;
  }

  if (program->pre_flush_sec > IRRIGATION_MAX_FLUSH_SEC) {
    program->pre_flush_sec = IRRIGATION_MAX_FLUSH_SEC;
  }

  if (program->post_flush_sec > IRRIGATION_MAX_FLUSH_SEC) {
    program->post_flush_sec = IRRIGATION_MAX_FLUSH_SEC;
  }

  IRRIGATION_CTRL_NormalizeProgramRecipe(program);
}

static void IRRIGATION_CTRL_NormalizeProgramRecipe(irrigation_program_t *program) {
  uint16_t ratio_sum = 0U;

  if (program == NULL) {
    return;
  }

  for (uint8_t i = 0U; i < IRRIGATION_EC_CHANNEL_COUNT; i++) {
    if (program->fert_ratio_percent[i] > 100U) {
      program->fert_ratio_percent[i] = 100U;
    }
    ratio_sum = (uint16_t)(ratio_sum + program->fert_ratio_percent[i]);
  }

  if (ratio_sum == 0U) {
    for (uint8_t i = 0U; i < IRRIGATION_EC_CHANNEL_COUNT; i++) {
      program->fert_ratio_percent[i] = 25U;
    }
  }
}
