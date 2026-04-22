/**
 ******************************************************************************
 * @file           : irrigation_control.c
 * @brief          : Program-based irrigation control implementation
 ******************************************************************************
 */

#include "main.h"
#include "irrigation_control.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef enum {
  DOSE_PHASE_IDLE = 0,
  DOSE_PHASE_PH,
  DOSE_PHASE_EC,
  DOSE_PHASE_MIXING,
  DOSE_PHASE_SETTLING
} dose_phase_t;

static irrigation_control_t ctrl = {0};
static auto_mode_t g_auto_mode = AUTO_MODE_SCHEDULED;
static irrigation_schedule_t g_schedules[VALVE_COUNT] = {0};
static irrigation_program_t g_programs[VALVE_COUNT] = {0};
static irrigation_runtime_backup_t g_runtime_backup = {0};
static eeprom_system_t g_system_record = {0};
static program_state_t g_program_state = PROGRAM_STATE_IDLE;
static dose_phase_t g_dose_phase = DOSE_PHASE_IDLE;
static uint8_t g_active_dosing_valve = 0U;
static uint8_t g_active_program_id = 0U;
static uint8_t g_manual_sequence_active = 0U;
static uint8_t g_active_valves[VALVE_COUNT] = {0};
static uint8_t g_active_valve_count = 0U;
static uint8_t g_active_valve_index = 0U;
static uint8_t g_repeat_index = 0U;
static uint8_t g_program_dirty_mask = 0U;
static uint8_t g_system_dirty = 0U;
static uint8_t g_runtime_dirty = 0U;
static uint32_t g_dose_started_ms = 0U;
static uint32_t g_mix_started_ms = 0U;
static uint32_t g_settle_started_ms = 0U;
static uint32_t g_wait_started_ms = 0U;
static uint32_t g_last_runtime_snapshot_ms = 0U;
static char g_alarm_text[32] = "READY";

static void IRRIGATION_CTRL_ApplySystemRecord(void);
static void IRRIGATION_CTRL_SetDefaults(void);
static auto_mode_t IRRIGATION_CTRL_GetStoredAutoMode(void);
static void IRRIGATION_CTRL_StoreAutoMode(auto_mode_t mode);
static void IRRIGATION_CTRL_PersistSystemRecord(void);
static void IRRIGATION_CTRL_PersistProgram(uint8_t program_id);
static void IRRIGATION_CTRL_PersistRuntimeBackup(void);
static void IRRIGATION_CTRL_ChangeState(control_state_t new_state);
static void IRRIGATION_CTRL_SetProgramState(program_state_t state);
static void IRRIGATION_CTRL_ResetCurrentParcel(void);
static void IRRIGATION_CTRL_ResetSequence(void);
static uint8_t IRRIGATION_CTRL_BuildSequenceFromMask(uint8_t valve_mask);
static uint8_t IRRIGATION_CTRL_BuildSequenceFromQueue(void);
static uint8_t IRRIGATION_CTRL_StartCurrentValve(uint32_t duration_sec);
static uint32_t IRRIGATION_CTRL_GetCurrentValveDurationSec(void);
static uint32_t IRRIGATION_CTRL_GetCurrentWaitDurationSec(void);
static uint8_t IRRIGATION_CTRL_HasMoreValvesInCycle(void);
static uint8_t IRRIGATION_CTRL_HasMoreCycleRepeats(void);
static uint16_t IRRIGATION_CTRL_GetCurrentHHMM(void);
static uint16_t IRRIGATION_CTRL_GetCurrentMinuteOfDay(void);
static uint16_t IRRIGATION_CTRL_GetCurrentDayStamp(void);
static uint16_t IRRIGATION_CTRL_HHMMToMinuteOfDay(uint16_t hhmm);
static uint8_t IRRIGATION_CTRL_IsMinuteWithinWindow(uint16_t minute_of_day,
                                                    uint16_t start_hhmm,
                                                    uint16_t end_hhmm);
static uint8_t IRRIGATION_CTRL_IsDayEnabled(uint8_t days_mask);
static void IRRIGATION_CTRL_CompleteCurrentValve(void);
static uint8_t IRRIGATION_CTRL_AdvanceSequence(void);
static void IRRIGATION_CTRL_FinishRun(void);
static void IRRIGATION_CTRL_ServiceValveActive(void);
static void IRRIGATION_CTRL_ServiceWaiting(void);
static void IRRIGATION_CTRL_ServiceDosing(void);
static void IRRIGATION_CTRL_StartDose(uint8_t valve_id, dose_phase_t phase,
                                      uint32_t duration_ms);
static void IRRIGATION_CTRL_StopDose(void);
static void IRRIGATION_CTRL_UpdateRuntimeBackup(void);
static void IRRIGATION_CTRL_SetAlarmText(const char *text);
static uint8_t IRRIGATION_CTRL_IsParcelQueued(uint8_t parcel_id);

void IRRIGATION_CTRL_Init(void) {
  memset(&ctrl, 0, sizeof(ctrl));
  memset(&g_runtime_backup, 0, sizeof(g_runtime_backup));
  memset(g_schedules, 0, sizeof(g_schedules));
  memset(g_programs, 0, sizeof(g_programs));

  IRRIGATION_CTRL_SetDefaults();
  IRRIGATION_CTRL_ApplySystemRecord();
  IRRIGATION_CTRL_LoadPrograms();
  IRRIGATION_CTRL_ChangeState(CTRL_STATE_IDLE);
  IRRIGATION_CTRL_SetProgramState(PROGRAM_STATE_IDLE);
  IRRIGATION_CTRL_LoadRuntimeBackup();
}

void IRRIGATION_CTRL_SetAutoMode(auto_mode_t mode) {
  g_auto_mode = mode;
  IRRIGATION_CTRL_StoreAutoMode(mode);
  g_system_dirty = 1U;
  IRRIGATION_CTRL_PersistSystemRecord();
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
  g_system_dirty = 1U;
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
  g_system_dirty = 1U;
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

void IRRIGATION_CTRL_SetParcelDuration(uint8_t parcel_id, uint32_t duration_sec) {
  if (duration_sec == 0U) {
    duration_sec = 30U;
  }

  PARCELS_SetDuration(parcel_id, duration_sec);
}

void IRRIGATION_CTRL_Start(void) {
  if (ctrl.is_running != 0U) {
    return;
  }

  if (ctrl.queue_size == 0U) {
    for (uint8_t parcel_id = 1U; parcel_id <= VALVE_COUNT; parcel_id++) {
      if (PARCELS_IsEnabled(parcel_id) != 0U) {
        IRRIGATION_CTRL_AddToQueue(parcel_id);
      }
    }
  }

  if (!IRRIGATION_CTRL_BuildSequenceFromQueue()) {
    return;
  }

  g_manual_sequence_active = 1U;
  g_active_program_id = 0U;
  g_repeat_index = 0U;
  ctrl.is_running = 1U;
  ctrl.is_paused = 0U;
  IRRIGATION_CTRL_SetProgramState(PROGRAM_STATE_VALVE_ACTIVE);
  IRRIGATION_CTRL_ChangeState(CTRL_STATE_PARCEL_WATERING);
  (void)IRRIGATION_CTRL_StartCurrentValve(IRRIGATION_CTRL_GetCurrentValveDurationSec());
}

void IRRIGATION_CTRL_StartProgram(uint8_t program_id) {
  irrigation_program_t *program;

  if (program_id == 0U || program_id > VALVE_COUNT) {
    return;
  }

  program = &g_programs[program_id - 1U];
  if (program->enabled == 0U) {
    return;
  }

  if (!IRRIGATION_CTRL_BuildSequenceFromMask(program->valve_mask)) {
    return;
  }

  g_manual_sequence_active = 0U;
  g_active_program_id = program_id;
  g_active_valve_index = 0U;
  g_repeat_index = 0U;
  ctrl.is_running = 1U;
  ctrl.is_paused = 0U;
  ctrl.last_error = ERR_NONE;
  ctrl.error_flags = 0U;
  ctrl.ph_params.target = (float)program->ph_set_x100 / 100.0f;
  ctrl.ec_params.target = (float)program->ec_set_x100 / 100.0f;
  IRRIGATION_CTRL_SetProgramState(PROGRAM_STATE_VALVE_ACTIVE);
  IRRIGATION_CTRL_ChangeState(CTRL_STATE_PARCEL_WATERING);
  (void)IRRIGATION_CTRL_StartCurrentValve(IRRIGATION_CTRL_GetCurrentValveDurationSec());
}

void IRRIGATION_CTRL_CancelProgram(void) { IRRIGATION_CTRL_Stop(); }

uint8_t IRRIGATION_CTRL_GetActiveProgram(void) { return g_active_program_id; }

program_state_t IRRIGATION_CTRL_GetProgramState(void) { return g_program_state; }

void IRRIGATION_CTRL_Stop(void) {
  ctrl.is_running = 0U;
  ctrl.is_paused = 0U;
  IRRIGATION_CTRL_StopDose();
  VALVES_CloseAll();
  IRRIGATION_CTRL_ResetCurrentParcel();
  IRRIGATION_CTRL_ResetSequence();
  IRRIGATION_CTRL_SetProgramState(PROGRAM_STATE_IDLE);
  IRRIGATION_CTRL_ChangeState(CTRL_STATE_IDLE);
  g_manual_sequence_active = 0U;
  g_active_program_id = 0U;
  (void)EEPROM_ClearRuntimeBackup();
  memset(&g_runtime_backup, 0, sizeof(g_runtime_backup));
}

void IRRIGATION_CTRL_Pause(void) { ctrl.is_paused = 1U; }

void IRRIGATION_CTRL_Resume(void) { ctrl.is_paused = 0U; }

uint8_t IRRIGATION_CTRL_IsRunning(void) { return ctrl.is_running; }

void IRRIGATION_CTRL_Update(void) {
  if (ctrl.is_running == 0U || ctrl.is_paused != 0U) {
    return;
  }

  switch (g_program_state) {
  case PROGRAM_STATE_VALVE_ACTIVE:
    IRRIGATION_CTRL_ServiceValveActive();
    break;
  case PROGRAM_STATE_WAITING:
    IRRIGATION_CTRL_ServiceWaiting();
    break;
  case PROGRAM_STATE_NEXT_VALVE:
    if (!IRRIGATION_CTRL_AdvanceSequence()) {
      IRRIGATION_CTRL_FinishRun();
      break;
    }
    IRRIGATION_CTRL_SetProgramState(PROGRAM_STATE_VALVE_ACTIVE);
    IRRIGATION_CTRL_ChangeState(CTRL_STATE_PARCEL_WATERING);
    (void)IRRIGATION_CTRL_StartCurrentValve(IRRIGATION_CTRL_GetCurrentValveDurationSec());
    break;
  case PROGRAM_STATE_ERROR:
    IRRIGATION_CTRL_EmergencyStop();
    break;
  case PROGRAM_STATE_IDLE:
  default:
    break;
  }

  if (g_active_program_id != 0U && ctrl.is_running != 0U) {
    irrigation_program_t *program = &g_programs[g_active_program_id - 1U];
    if (program->end_hhmm != 0U &&
        IRRIGATION_CTRL_GetCurrentHHMM() > program->end_hhmm) {
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
  IRRIGATION_CTRL_SetProgramState(PROGRAM_STATE_NEXT_VALVE);
}

uint8_t IRRIGATION_CTRL_IsParcelComplete(void) {
  return (IRRIGATION_CTRL_GetRemainingTime() == 0U) ? 1U : 0U;
}

uint8_t IRRIGATION_CTRL_GetCurrentParcelId(void) {
  return ctrl.current_parcel.parcel_id;
}

uint32_t IRRIGATION_CTRL_GetRemainingTime(void) {
  uint32_t duration_sec;
  uint32_t elapsed_sec;
  uint32_t now = HAL_GetTick();

  if (ctrl.is_running == 0U) {
    return 0U;
  }

  if (g_program_state == PROGRAM_STATE_WAITING) {
    duration_sec = IRRIGATION_CTRL_GetCurrentWaitDurationSec();
    elapsed_sec = (now - g_wait_started_ms) / 1000U;
    return (elapsed_sec >= duration_sec) ? 0U : (duration_sec - elapsed_sec);
  }

  if (ctrl.current_parcel.parcel_id == 0U) {
    return 0U;
  }

  duration_sec = ctrl.current_parcel.duration_sec;
  elapsed_sec = (now - ctrl.current_parcel.start_time) / 1000U;
  return (elapsed_sec >= duration_sec) ? 0U : (duration_sec - elapsed_sec);
}

void IRRIGATION_CTRL_AddToQueue(uint8_t parcel_id) {
  if (parcel_id == 0U || parcel_id > VALVE_COUNT || ctrl.queue_size >= VALVE_COUNT) {
    return;
  }

  if (PARCELS_IsEnabled(parcel_id) == 0U || IRRIGATION_CTRL_IsParcelQueued(parcel_id) != 0U) {
    return;
  }

  ctrl.parcel_queue[ctrl.queue_tail] = parcel_id;
  ctrl.queue_tail = (ctrl.queue_tail + 1U) % VALVE_COUNT;
  ctrl.queue_size++;
}

void IRRIGATION_CTRL_ClearQueue(void) {
  memset(ctrl.parcel_queue, 0, sizeof(ctrl.parcel_queue));
  ctrl.queue_head = 0U;
  ctrl.queue_tail = 0U;
  ctrl.queue_size = 0U;
}

void IRRIGATION_CTRL_StartMixing(void) {
  g_dose_phase = DOSE_PHASE_MIXING;
  g_mix_started_ms = HAL_GetTick();
  IRRIGATION_CTRL_ChangeState(CTRL_STATE_MIXING);
}

void IRRIGATION_CTRL_StopMixing(void) {
  if (g_dose_phase == DOSE_PHASE_MIXING || g_dose_phase == DOSE_PHASE_SETTLING) {
    g_dose_phase = DOSE_PHASE_IDLE;
  }
}

uint8_t IRRIGATION_CTRL_IsMixingComplete(void) {
  uint32_t wait_ms = (g_dose_phase == DOSE_PHASE_EC) ? ctrl.ec_params.wait_time_ms
                                                     : ctrl.ph_params.wait_time_ms;
  if (wait_ms == 0U) {
    wait_ms = IRRIGATION_MIXING_TIME * 1000U;
  }
  return ((HAL_GetTick() - g_mix_started_ms) >= wait_ms) ? 1U : 0U;
}

void IRRIGATION_CTRL_StartSettling(void) {
  g_dose_phase = DOSE_PHASE_SETTLING;
  g_settle_started_ms = HAL_GetTick();
  IRRIGATION_CTRL_ChangeState(CTRL_STATE_SETTLING);
}

uint8_t IRRIGATION_CTRL_IsSettlingComplete(void) {
  return ((HAL_GetTick() - g_settle_started_ms) >=
          (IRRIGATION_SETTLING_TIME * 1000U))
             ? 1U
             : 0U;
}

void IRRIGATION_CTRL_SetError(error_code_t error) {
  ctrl.last_error = error;
  ctrl.error_flags = 1U;
  ctrl.is_running = 0U;
  ctrl.is_paused = 0U;
  IRRIGATION_CTRL_StopDose();
  VALVES_EmergencyStop();
  IRRIGATION_CTRL_SetProgramState(PROGRAM_STATE_ERROR);
  IRRIGATION_CTRL_ChangeState(CTRL_STATE_ERROR);
  IRRIGATION_CTRL_SetAlarmText(IRRIGATION_CTRL_GetErrorString(error));
  g_runtime_dirty = 1U;
  IRRIGATION_CTRL_OnError(error);
}

void IRRIGATION_CTRL_ClearError(error_code_t error) {
  if (ctrl.last_error == error) {
    ctrl.last_error = ERR_NONE;
  }
  if (ctrl.last_error == ERR_NONE) {
    ctrl.error_flags = 0U;
  }
}

void IRRIGATION_CTRL_ClearAllErrors(void) {
  ctrl.error_flags = 0U;
  ctrl.last_error = ERR_NONE;
  IRRIGATION_CTRL_SetAlarmText("READY");
}

error_code_t IRRIGATION_CTRL_GetLastError(void) { return ctrl.last_error; }

uint8_t IRRIGATION_CTRL_HasErrors(void) { return ctrl.error_flags; }

const char *IRRIGATION_CTRL_GetErrorString(error_code_t error) {
  switch (error) {
  case ERR_PH_SENSOR_FAULT:
    return "PH_SENSOR";
  case ERR_EC_SENSOR_FAULT:
    return "EC_SENSOR";
  case ERR_PH_OUT_OF_RANGE:
    return "PH_RANGE";
  case ERR_EC_OUT_OF_RANGE:
    return "EC_RANGE";
  case ERR_VALVE_STUCK:
    return "VALVE";
  case ERR_TIMEOUT:
    return "TIMEOUT";
  case ERR_LOW_WATER_LEVEL:
    return "LOW_WATER";
  case ERR_OVERCURRENT:
    return "OVERCURRENT";
  case ERR_COMMUNICATION:
    return "COMM";
  case ERR_NONE:
  default:
    return "NONE";
  }
}

void IRRIGATION_CTRL_EmergencyStop(void) {
  IRRIGATION_CTRL_StopDose();
  VALVES_EmergencyStop();
  ctrl.is_running = 0U;
  ctrl.error_flags = 1U;
  if (ctrl.last_error == ERR_NONE) {
    ctrl.last_error = ERR_TIMEOUT;
  }
  IRRIGATION_CTRL_SetProgramState(PROGRAM_STATE_ERROR);
  IRRIGATION_CTRL_ChangeState(CTRL_STATE_EMERGENCY_STOP);
  g_runtime_dirty = 1U;
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

  if (slot >= VALVE_COUNT || sched == NULL) {
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
  if (program.repeat_count == 0U) {
    program.repeat_count = 1U;
  }
  if (program.end_hhmm == 0U) {
    program.end_hhmm = (uint16_t)(program.start_hhmm + 100U);
  }
  IRRIGATION_CTRL_SetProgram(slot + 1U, &program);
}

void IRRIGATION_CTRL_GetSchedule(uint8_t slot, irrigation_schedule_t *sched) {
  if (slot >= VALVE_COUNT || sched == NULL) {
    return;
  }

  if (g_schedules[slot].enabled == 0U && g_programs[slot].enabled != 0U) {
    g_schedules[slot].enabled = g_programs[slot].enabled;
    g_schedules[slot].hour = (uint8_t)(g_programs[slot].start_hhmm / 100U);
    g_schedules[slot].minute = (uint8_t)(g_programs[slot].start_hhmm % 100U);
    g_schedules[slot].days_of_week = g_programs[slot].days_mask;
    for (uint8_t i = 0U; i < VALVE_COUNT; i++) {
      if ((g_programs[slot].valve_mask & (1U << i)) != 0U) {
        g_schedules[slot].parcel_id = i + 1U;
        break;
      }
    }
  }

  *sched = g_schedules[slot];
}

void IRRIGATION_CTRL_CheckSchedules(void) {
  uint16_t minute_of_day;
  uint16_t day_stamp;

  if (ctrl.is_running != 0U) {
    return;
  }

  if (g_auto_mode != AUTO_MODE_SCHEDULED && g_auto_mode != AUTO_MODE_FULL_AUTO) {
    return;
  }

  if (RTC_IsInitialized() == 0U) {
    return;
  }

  minute_of_day = IRRIGATION_CTRL_GetCurrentMinuteOfDay();
  day_stamp = IRRIGATION_CTRL_GetCurrentDayStamp();

  for (uint8_t i = 0U; i < VALVE_COUNT; i++) {
    irrigation_program_t *program = &g_programs[i];

    if (program->enabled == 0U) {
      continue;
    }

    if (!IRRIGATION_CTRL_IsDayEnabled(program->days_mask)) {
      continue;
    }

    if (!IRRIGATION_CTRL_IsMinuteWithinWindow(minute_of_day, program->start_hhmm,
                                              program->end_hhmm)) {
      continue;
    }

    if (program->last_run_day == day_stamp) {
      continue;
    }

    program->last_run_day = day_stamp;
    program->last_run_minute = minute_of_day;
    g_program_dirty_mask |= (uint8_t)(1U << i);
    IRRIGATION_CTRL_StartProgram(i + 1U);
    break;
  }
}

void IRRIGATION_CTRL_GetProgram(uint8_t program_id, irrigation_program_t *program) {
  if (program_id == 0U || program_id > VALVE_COUNT || program == NULL) {
    return;
  }

  *program = g_programs[program_id - 1U];
}

void IRRIGATION_CTRL_SetProgram(uint8_t program_id,
                                const irrigation_program_t *program) {
  if (program_id == 0U || program_id > VALVE_COUNT || program == NULL) {
    return;
  }

  g_programs[program_id - 1U] = *program;
  g_programs[program_id - 1U].crc =
      EEPROM_CalculateCRC(&g_programs[program_id - 1U],
                          sizeof(irrigation_program_t) - 2U);
  g_program_dirty_mask |= (uint8_t)(1U << (program_id - 1U));
  IRRIGATION_CTRL_PersistProgram(program_id);
}

void IRRIGATION_CTRL_LoadPrograms(void) {
  (void)EEPROM_LoadAllPrograms(g_programs, VALVE_COUNT);
}

void IRRIGATION_CTRL_LoadRuntimeBackup(void) {
  irrigation_program_t *program;
  uint32_t duration_sec;

  if (EEPROM_LoadRuntimeBackup(&g_runtime_backup) != EEPROM_OK) {
    memset(&g_runtime_backup, 0, sizeof(g_runtime_backup));
    return;
  }

  if (g_runtime_backup.active_program_id == 0U ||
      g_runtime_backup.active_program_id > VALVE_COUNT) {
    return;
  }

  program = &g_programs[g_runtime_backup.active_program_id - 1U];
  if (program->enabled == 0U ||
      !IRRIGATION_CTRL_BuildSequenceFromMask(program->valve_mask)) {
    return;
  }

  g_active_program_id = g_runtime_backup.active_program_id;
  g_active_valve_index = g_runtime_backup.active_valve_index;
  g_repeat_index = g_runtime_backup.repeat_index;
  ctrl.is_running = 1U;
  ctrl.is_paused = 0U;
  ctrl.last_error = (error_code_t)g_runtime_backup.error_code;
  ctrl.ph_params.target = (float)g_runtime_backup.ph_target_x100 / 100.0f;
  ctrl.ec_params.target = (float)g_runtime_backup.ec_target_x100 / 100.0f;

  if (g_active_valve_index >= g_active_valve_count) {
    IRRIGATION_CTRL_Stop();
    return;
  }

  duration_sec = IRRIGATION_CTRL_GetCurrentValveDurationSec();
  if (g_runtime_backup.remaining_sec > duration_sec) {
    g_runtime_backup.remaining_sec = (uint16_t)duration_sec;
  }

  IRRIGATION_CTRL_SetProgramState((program_state_t)g_runtime_backup.program_state);
  if (g_program_state == PROGRAM_STATE_WAITING) {
    g_wait_started_ms =
        HAL_GetTick() - ((duration_sec - g_runtime_backup.remaining_sec) * 1000UL);
    IRRIGATION_CTRL_ChangeState(CTRL_STATE_WAITING);
    return;
  }

  IRRIGATION_CTRL_ChangeState(CTRL_STATE_PARCEL_WATERING);
  if (IRRIGATION_CTRL_StartCurrentValve(duration_sec) == 0U) {
    IRRIGATION_CTRL_Stop();
    return;
  }

  ctrl.current_parcel.start_time =
      HAL_GetTick() - ((duration_sec - g_runtime_backup.remaining_sec) * 1000UL);
}

void IRRIGATION_CTRL_MaintenanceTask(void) {
  if (g_system_dirty != 0U) {
    (void)EEPROM_SetSystemParams(&g_system_record);
    g_system_dirty = 0U;
  }

  for (uint8_t i = 0U; i < VALVE_COUNT; i++) {
    if ((g_program_dirty_mask & (1U << i)) == 0U) continue;
    (void)EEPROM_SaveProgram(i + 1U, &g_programs[i]);
    g_program_dirty_mask &= (uint8_t)~(1U << i);
  }

  if (ctrl.is_running != 0U &&
      (HAL_GetTick() - g_last_runtime_snapshot_ms) >=
          SYSTEM_RUNTIME_SNAPSHOT_PERIOD_MS) {
    g_last_runtime_snapshot_ms = HAL_GetTick();
    g_runtime_dirty = 1U;
  }

  if (g_runtime_dirty != 0U) {
    IRRIGATION_CTRL_PersistRuntimeBackup();
  }
}

uint8_t IRRIGATION_CTRL_RunSafetyChecks(void) {
  uint32_t now = HAL_GetTick();

  if (RTC_IsInitialized() == 0U) {
    IRRIGATION_CTRL_SetError(ERR_COMMUNICATION);
    return 0U;
  }

  if ((now - PH_GetLastReadTime()) > SYSTEM_SENSOR_STALE_TIMEOUT_MS) {
    IRRIGATION_CTRL_SetError(ERR_PH_SENSOR_FAULT);
    return 0U;
  }

  if ((now - EC_GetLastReadTime()) > SYSTEM_SENSOR_STALE_TIMEOUT_MS) {
    IRRIGATION_CTRL_SetError(ERR_EC_SENSOR_FAULT);
    return 0U;
  }

  if (g_active_dosing_valve != 0U &&
      ctrl.current_parcel.valve_id != 0U &&
      g_active_dosing_valve == ctrl.current_parcel.valve_id) {
    IRRIGATION_CTRL_SetError(ERR_VALVE_STUCK);
    return 0U;
  }

  return 1U;
}

const char *IRRIGATION_CTRL_GetActiveAlarmText(void) { return g_alarm_text; }

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
  if (EEPROM_LoadSystemParams() == EEPROM_OK) {
    (void)EEPROM_GetSystemParams(&g_system_record);
  }

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
  ctrl.ph_params.acid_valve_id = 0U;
  ctrl.ph_params.base_valve_id = 0U;

  ctrl.ec_params.target = 1.80f;
  ctrl.ec_params.min_limit = 1.00f;
  ctrl.ec_params.max_limit = 2.50f;
  ctrl.ec_params.hysteresis = IRRIGATION_EC_HYSTERESIS;
  ctrl.ec_params.dose_time_ms = 500U;
  ctrl.ec_params.wait_time_ms = IRRIGATION_MIXING_TIME * 1000U;
  ctrl.ec_params.auto_adjust_enabled = 1U;
  ctrl.ec_params.fertilizer_valve_id = 0U;
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

static void IRRIGATION_CTRL_PersistSystemRecord(void) {
  if (g_system_dirty == 0U) {
    return;
  }

  if (EEPROM_SetSystemParams(&g_system_record) == EEPROM_OK) {
    g_system_dirty = 0U;
  }
}

static void IRRIGATION_CTRL_PersistProgram(uint8_t program_id) {
  uint8_t program_index;
  uint8_t bit;

  if (program_id == 0U || program_id > VALVE_COUNT) {
    return;
  }

  program_index = (uint8_t)(program_id - 1U);
  bit = (uint8_t)(1U << program_index);

  if ((g_program_dirty_mask & bit) == 0U) {
    return;
  }

  if (EEPROM_SaveProgram(program_id, &g_programs[program_index]) == EEPROM_OK) {
    g_program_dirty_mask &= (uint8_t)~bit;
  }
}

static void IRRIGATION_CTRL_PersistRuntimeBackup(void) {
  if (g_runtime_dirty == 0U) {
    return;
  }

  IRRIGATION_CTRL_UpdateRuntimeBackup();
  if (ctrl.is_running != 0U) {
    if (EEPROM_SaveRuntimeBackup(&g_runtime_backup) == EEPROM_OK) {
      g_runtime_dirty = 0U;
    }
  } else if (EEPROM_ClearRuntimeBackup() == EEPROM_OK) {
    g_runtime_dirty = 0U;
  }
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

static void IRRIGATION_CTRL_SetProgramState(program_state_t state) {
  g_program_state = state;
  g_runtime_dirty = 1U;
}

static void IRRIGATION_CTRL_ResetCurrentParcel(void) {
  memset(&ctrl.current_parcel, 0, sizeof(ctrl.current_parcel));
  GUI_UpdateIrrigationStatus(ctrl.is_running, 0U);
}

static void IRRIGATION_CTRL_ResetSequence(void) {
  memset(g_active_valves, 0, sizeof(g_active_valves));
  g_active_valve_count = 0U;
  g_active_valve_index = 0U;
  g_repeat_index = 0U;
  g_wait_started_ms = 0U;
}

static uint8_t IRRIGATION_CTRL_BuildSequenceFromMask(uint8_t valve_mask) {
  g_active_valve_count = 0U;
  memset(g_active_valves, 0, sizeof(g_active_valves));

  for (uint8_t i = 0U; i < VALVE_COUNT; i++) {
    if ((valve_mask & (1U << i)) == 0U) continue;
    g_active_valves[g_active_valve_count++] = i + 1U;
  }

  g_active_valve_index = 0U;
  return (g_active_valve_count > 0U) ? 1U : 0U;
}

static uint8_t IRRIGATION_CTRL_BuildSequenceFromQueue(void) {
  uint8_t index = ctrl.queue_head;
  uint8_t count = ctrl.queue_size;

  g_active_valve_count = 0U;
  memset(g_active_valves, 0, sizeof(g_active_valves));

  while (count > 0U) {
    uint8_t parcel_id = ctrl.parcel_queue[index];
    if (parcel_id != 0U && PARCELS_IsEnabled(parcel_id) != 0U) {
      g_active_valves[g_active_valve_count++] = parcel_id;
    }

    index = (index + 1U) % VALVE_COUNT;
    count--;
  }

  g_active_valve_index = 0U;
  return (g_active_valve_count > 0U) ? 1U : 0U;
}

static uint8_t IRRIGATION_CTRL_StartCurrentValve(uint32_t duration_sec) {
  uint8_t valve_id;

  if (g_active_valve_index >= g_active_valve_count) {
    return 0U;
  }

  valve_id = g_active_valves[g_active_valve_index];
  ctrl.current_parcel.parcel_id = valve_id;
  ctrl.current_parcel.valve_id = valve_id;
  ctrl.current_parcel.start_time = HAL_GetTick();
  ctrl.current_parcel.duration_sec = duration_sec;
  ctrl.current_parcel.elapsed_sec = 0U;
  ctrl.current_parcel.is_complete = 0U;
  ctrl.current_parcel.is_skipped = 0U;
  VALVES_Open(valve_id);
  GUI_UpdateIrrigationStatus(ctrl.is_running, valve_id);
  g_runtime_dirty = 1U;
  IRRIGATION_CTRL_PersistRuntimeBackup();
  return 1U;
}

static uint32_t IRRIGATION_CTRL_GetCurrentValveDurationSec(void) {
  if (g_manual_sequence_active != 0U) {
    return PARCELS_GetDuration(g_active_valves[g_active_valve_index]);
  }

  if (g_active_program_id != 0U) {
    uint32_t duration = (uint32_t)g_programs[g_active_program_id - 1U].irrigation_min * 60UL;
    return (duration == 0U) ? 60U : duration;
  }

  return 60U;
}

static uint32_t IRRIGATION_CTRL_GetCurrentWaitDurationSec(void) {
  if (g_manual_sequence_active != 0U) {
    return 0U;
  }

  if (g_active_program_id != 0U) {
    return (uint32_t)g_programs[g_active_program_id - 1U].wait_min * 60UL;
  }

  return 0U;
}

static uint8_t IRRIGATION_CTRL_HasMoreValvesInCycle(void) {
  return ((g_active_valve_index + 1U) < g_active_valve_count) ? 1U : 0U;
}

static uint8_t IRRIGATION_CTRL_HasMoreCycleRepeats(void) {
  uint8_t repeat_total = 1U;

  if (g_active_program_id != 0U) {
    repeat_total = g_programs[g_active_program_id - 1U].repeat_count;
    if (repeat_total == 0U) {
      repeat_total = 1U;
    }
  }

  return ((g_repeat_index + 1U) < repeat_total) ? 1U : 0U;
}

static uint16_t IRRIGATION_CTRL_GetCurrentHHMM(void) {
  rtc_time_t time = {0};
  RTC_GetTime(&time);
  return (uint16_t)((time.hours * 100U) + time.minutes);
}

static uint16_t IRRIGATION_CTRL_GetCurrentMinuteOfDay(void) {
  rtc_time_t time = {0};
  RTC_GetTime(&time);
  return (uint16_t)((time.hours * 60U) + time.minutes);
}

static uint16_t IRRIGATION_CTRL_GetCurrentDayStamp(void) {
  rtc_date_t date = {0};
  RTC_GetDate(&date);
  return (uint16_t)((date.year * 512U) + (date.month * 32U) + date.date);
}

static uint16_t IRRIGATION_CTRL_HHMMToMinuteOfDay(uint16_t hhmm) {
  uint16_t hours = (uint16_t)(hhmm / 100U);
  uint16_t minutes = (uint16_t)(hhmm % 100U);

  if (hours >= 24U || minutes >= 60U) {
    return 0U;
  }

  return (uint16_t)((hours * 60U) + minutes);
}

static uint8_t IRRIGATION_CTRL_IsMinuteWithinWindow(uint16_t minute_of_day,
                                                    uint16_t start_hhmm,
                                                    uint16_t end_hhmm) {
  uint16_t start_minute = IRRIGATION_CTRL_HHMMToMinuteOfDay(start_hhmm);
  uint16_t end_minute = IRRIGATION_CTRL_HHMMToMinuteOfDay(end_hhmm);

  if (end_hhmm == 0U || start_minute == end_minute) {
    return (minute_of_day >= start_minute) ? 1U : 0U;
  }

  if (start_minute < end_minute) {
    return (minute_of_day >= start_minute && minute_of_day <= end_minute) ? 1U : 0U;
  }

  return (minute_of_day >= start_minute || minute_of_day <= end_minute) ? 1U : 0U;
}

static uint8_t IRRIGATION_CTRL_IsDayEnabled(uint8_t days_mask) {
  rtc_date_t date = {0};
  uint8_t bit;

  RTC_GetDate(&date);
  if (date.weekday < 1U || date.weekday > 7U) {
    return 0U;
  }

  bit = (uint8_t)(date.weekday - 1U);
  return ((days_mask & (1U << bit)) != 0U) ? 1U : 0U;
}

static void IRRIGATION_CTRL_CompleteCurrentValve(void) {
  VALVES_Close(ctrl.current_parcel.valve_id);
  ctrl.total_parcel_count++;
  ctrl.total_watering_time += ctrl.current_parcel.duration_sec;
  ctrl.current_parcel.is_complete = 1U;
  IRRIGATION_CTRL_OnParcelComplete(ctrl.current_parcel.parcel_id);
  IRRIGATION_CTRL_StopDose();
  g_runtime_dirty = 1U;
}

static uint8_t IRRIGATION_CTRL_AdvanceSequence(void) {
  uint8_t repeat_total = 1U;

  if (g_active_program_id != 0U) {
    repeat_total = g_programs[g_active_program_id - 1U].repeat_count;
    if (repeat_total == 0U) {
      repeat_total = 1U;
    }
  }

  if ((g_active_valve_index + 1U) < g_active_valve_count) {
    g_active_valve_index++;
    return 1U;
  }

  if ((g_repeat_index + 1U) < repeat_total) {
    g_repeat_index++;
    g_active_valve_index = 0U;
    return 1U;
  }

  return 0U;
}

static void IRRIGATION_CTRL_FinishRun(void) {
  IRRIGATION_CTRL_Stop();
  IRRIGATION_CTRL_SetAlarmText("PROGRAM DONE");
}

static void IRRIGATION_CTRL_ServiceValveActive(void) {
  uint32_t elapsed_sec;

  IRRIGATION_CTRL_ReadSensors();
  IRRIGATION_CTRL_ServiceDosing();

  elapsed_sec = (HAL_GetTick() - ctrl.current_parcel.start_time) / 1000U;
  ctrl.current_parcel.elapsed_sec = elapsed_sec;
  if (elapsed_sec < ctrl.current_parcel.duration_sec) {
    return;
  }

  IRRIGATION_CTRL_CompleteCurrentValve();

  if (IRRIGATION_CTRL_HasMoreValvesInCycle() != 0U) {
    IRRIGATION_CTRL_SetProgramState(PROGRAM_STATE_NEXT_VALVE);
    return;
  }

  if (IRRIGATION_CTRL_HasMoreCycleRepeats() != 0U &&
      IRRIGATION_CTRL_GetCurrentWaitDurationSec() > 0U) {
    g_wait_started_ms = HAL_GetTick();
    IRRIGATION_CTRL_SetProgramState(PROGRAM_STATE_WAITING);
    IRRIGATION_CTRL_ChangeState(CTRL_STATE_WAITING);
    g_runtime_dirty = 1U;
    IRRIGATION_CTRL_PersistRuntimeBackup();
  } else {
    IRRIGATION_CTRL_SetProgramState(PROGRAM_STATE_NEXT_VALVE);
  }
}

static void IRRIGATION_CTRL_ServiceWaiting(void) {
  if ((HAL_GetTick() - g_wait_started_ms) <
      (IRRIGATION_CTRL_GetCurrentWaitDurationSec() * 1000UL)) {
    return;
  }

  IRRIGATION_CTRL_SetProgramState(PROGRAM_STATE_NEXT_VALVE);
}

static void IRRIGATION_CTRL_ServiceDosing(void) {
  IRRIGATION_CTRL_ReadSensors();

  switch (g_dose_phase) {
  case DOSE_PHASE_IDLE:
    if (ctrl.ph_params.auto_adjust_enabled != 0U &&
        IRRIGATION_CTRL_IsPHInRange() == 0U) {
      uint8_t valve_id = (ctrl.ph_data.ph_value > ctrl.ph_params.target)
                             ? ctrl.ph_params.acid_valve_id
                             : ctrl.ph_params.base_valve_id;
      IRRIGATION_CTRL_StartDose(valve_id, DOSE_PHASE_PH,
                                ctrl.ph_params.dose_time_ms);
      if (valve_id == 0U) {
        IRRIGATION_CTRL_StartMixing();
      } else {
        IRRIGATION_CTRL_ChangeState(CTRL_STATE_PH_ADJUSTING);
      }
      return;
    }

    if (ctrl.ec_params.auto_adjust_enabled != 0U &&
        ctrl.ec_data.ec_value < ctrl.ec_params.target - ctrl.ec_params.hysteresis) {
      IRRIGATION_CTRL_StartDose(ctrl.ec_params.fertilizer_valve_id, DOSE_PHASE_EC,
                                ctrl.ec_params.dose_time_ms);
      if (ctrl.ec_params.fertilizer_valve_id == 0U) {
        IRRIGATION_CTRL_StartMixing();
      } else {
        IRRIGATION_CTRL_ChangeState(CTRL_STATE_EC_ADJUSTING);
      }
    }
    break;

  case DOSE_PHASE_PH:
  case DOSE_PHASE_EC:
    if ((HAL_GetTick() - g_dose_started_ms) >=
        ((g_dose_phase == DOSE_PHASE_PH) ? ctrl.ph_params.dose_time_ms
                                         : ctrl.ec_params.dose_time_ms)) {
      IRRIGATION_CTRL_StopDose();
      IRRIGATION_CTRL_StartMixing();
    }
    break;

  case DOSE_PHASE_MIXING:
    if (IRRIGATION_CTRL_IsMixingComplete() != 0U) {
      IRRIGATION_CTRL_StartSettling();
    }
    break;

  case DOSE_PHASE_SETTLING:
    if (IRRIGATION_CTRL_IsSettlingComplete() != 0U) {
      g_dose_phase = DOSE_PHASE_IDLE;
      IRRIGATION_CTRL_ChangeState(CTRL_STATE_PARCEL_WATERING);
    }
    break;

  default:
    g_dose_phase = DOSE_PHASE_IDLE;
    break;
  }
}

static void IRRIGATION_CTRL_StartDose(uint8_t valve_id, dose_phase_t phase,
                                      uint32_t duration_ms) {
  g_dose_phase = phase;
  g_dose_started_ms = HAL_GetTick();
  g_active_dosing_valve = valve_id;

  if (duration_ms == 0U) {
    duration_ms = 500U;
  }

  (void)duration_ms;
  if (valve_id != 0U) {
    VALVES_Open(valve_id);
  }
}

static void IRRIGATION_CTRL_StopDose(void) {
  if (g_active_dosing_valve != 0U) {
    VALVES_Close(g_active_dosing_valve);
  }
  g_active_dosing_valve = 0U;
  g_dose_phase = DOSE_PHASE_IDLE;
  g_dose_started_ms = 0U;
  g_mix_started_ms = 0U;
  g_settle_started_ms = 0U;
}

static void IRRIGATION_CTRL_UpdateRuntimeBackup(void) {
  memset(&g_runtime_backup, 0, sizeof(g_runtime_backup));
  g_runtime_backup.valid = (ctrl.is_running != 0U) ? 1U : 0U;
  g_runtime_backup.active_program_id = g_active_program_id;
  g_runtime_backup.active_valve_index = g_active_valve_index;
  g_runtime_backup.repeat_index = g_repeat_index;
  g_runtime_backup.program_state = (uint8_t)g_program_state;
  g_runtime_backup.active_valve_id = ctrl.current_parcel.valve_id;
  g_runtime_backup.remaining_sec = (uint16_t)IRRIGATION_CTRL_GetRemainingTime();
  g_runtime_backup.ec_target_x100 = (int16_t)(ctrl.ec_params.target * 100.0f);
  g_runtime_backup.ph_target_x100 = (int16_t)(ctrl.ph_params.target * 100.0f);
  g_runtime_backup.error_code = (uint8_t)ctrl.last_error;
  g_runtime_backup.crc =
      EEPROM_CalculateCRC(&g_runtime_backup, sizeof(g_runtime_backup) - 2U);
}

static void IRRIGATION_CTRL_SetAlarmText(const char *text) {
  if (text == NULL || text[0] == '\0') {
    snprintf(g_alarm_text, sizeof(g_alarm_text), "READY");
    return;
  }

  snprintf(g_alarm_text, sizeof(g_alarm_text), "%s", text);
}

static uint8_t IRRIGATION_CTRL_IsParcelQueued(uint8_t parcel_id) {
  uint8_t index = ctrl.queue_head;
  uint8_t remaining = ctrl.queue_size;

  while (remaining > 0U) {
    if (ctrl.parcel_queue[index] == parcel_id) {
      return 1U;
    }
    index = (index + 1U) % VALVE_COUNT;
    remaining--;
  }

  return 0U;
}
