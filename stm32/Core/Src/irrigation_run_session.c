/**
 ******************************************************************************
 * @file           : irrigation_run_session.c
 * @brief          : Aktif sulama oturumu runtime yardimcilari
 ******************************************************************************
 */

#include "irrigation_run_session.h"
#include "dosing_controller.h"
#include "gui.h"
#include "irrigation_runtime.h"
#include "parcel_scheduler.h"
#include "valves.h"
#include "main.h"
#include <string.h>

void IRRIGATION_RUN_Begin(irrigation_control_t *ctrl) {
  if (ctrl == NULL) {
    return;
  }

  ctrl->is_running = 1U;
  ctrl->is_paused = 0U;
}

void IRRIGATION_RUN_ApplyProgramTargets(irrigation_control_t *ctrl,
                                        const irrigation_program_t *program) {
  if (ctrl == NULL || program == NULL) {
    return;
  }

  ctrl->ph_params.target = (float)program->ph_set_x100 / 100.0f;
  ctrl->ec_params.target = (float)program->ec_set_x100 / 100.0f;
  if (program->learned_ph_pwm_percent > 0U &&
      program->learned_ph_pwm_percent <= 100U) {
    ctrl->ph_params.pwm_duty_percent = program->learned_ph_pwm_percent;
  }
  if (program->learned_ec_pwm_percent > 0U &&
      program->learned_ec_pwm_percent <= 100U) {
    ctrl->ec_params.pwm_duty_percent = program->learned_ec_pwm_percent;
  }
  memcpy(ctrl->ec_params.recipe_ratio, program->fert_ratio_percent,
         sizeof(ctrl->ec_params.recipe_ratio));
}

void IRRIGATION_RUN_Stop(irrigation_control_t *ctrl) {
  if (ctrl == NULL) {
    return;
  }

  ctrl->is_running = 0U;
  ctrl->is_paused = 0U;
  DOSING_CTRL_Reset();
  VALVES_CloseAll();
  IRRIGATION_RUN_ResetCurrentParcel(ctrl);
  PARCEL_SCHED_Reset();
  IRRIGATION_RUNTIME_Reset();
  IRRIGATION_RUNTIME_RequestPersist();
}

void IRRIGATION_RUN_Pause(irrigation_control_t *ctrl) {
  if (ctrl != NULL) {
    ctrl->is_paused = 1U;
  }
}

void IRRIGATION_RUN_Resume(irrigation_control_t *ctrl) {
  if (ctrl != NULL) {
    ctrl->is_paused = 0U;
  }
}

void IRRIGATION_RUN_ResetCurrentParcel(irrigation_control_t *ctrl) {
  if (ctrl == NULL) {
    return;
  }

  memset(&ctrl->current_parcel, 0, sizeof(ctrl->current_parcel));
  GUI_UpdateIrrigationStatus(ctrl->is_running, 0U);
}

uint8_t IRRIGATION_RUN_StartCurrentValve(irrigation_control_t *ctrl,
                                         const irrigation_program_t *programs,
                                         program_state_t phase) {
  uint8_t valve_id;

  if (ctrl == NULL || programs == NULL) {
    return 0U;
  }

  if (PARCEL_SCHED_GetActiveValveIndex() >= PARCEL_SCHED_GetActiveValveCount()) {
    return 0U;
  }

  valve_id = PARCEL_SCHED_GetCurrentValve();
  ctrl->current_parcel.parcel_id = valve_id;
  ctrl->current_parcel.valve_id = valve_id;
  ctrl->current_parcel.is_complete = 0U;
  ctrl->current_parcel.is_skipped = 0U;

  if (VALVES_RequestOpen(valve_id) == 0U) {
    ctrl->current_parcel.is_skipped = 1U;
    ctrl->current_parcel.parcel_id = 0U;
    ctrl->current_parcel.valve_id = 0U;
    return 0U;
  }

  GUI_UpdateIrrigationStatus(ctrl->is_running, valve_id);
  IRRIGATION_RUN_StartPhase(ctrl, programs, phase);
  IRRIGATION_RUNTIME_RequestPersist();
  return 1U;
}

uint8_t IRRIGATION_RUN_ActivateCurrentValve(irrigation_control_t *ctrl,
                                            const irrigation_program_t *programs,
                                            program_state_t phase) {
  if (ctrl == NULL || programs == NULL) {
    return 0U;
  }

  IRRIGATION_RUNTIME_SetProgramState(phase);
  return IRRIGATION_RUN_StartCurrentValve(ctrl, programs, phase);
}

void IRRIGATION_RUN_StartPhase(irrigation_control_t *ctrl,
                               const irrigation_program_t *programs,
                               program_state_t phase) {
  if (ctrl == NULL || programs == NULL) {
    return;
  }

  ctrl->current_parcel.start_time = HAL_GetTick();
  ctrl->current_parcel.duration_sec =
      IRRIGATION_RUN_GetCurrentPhaseDurationSec(programs, phase);
  ctrl->current_parcel.elapsed_sec = 0U;
  IRRIGATION_RUNTIME_SetProgramState(phase);
  IRRIGATION_RUNTIME_RequestPersist();
}

void IRRIGATION_RUN_RestoreCurrentValveTiming(irrigation_control_t *ctrl,
                                              uint32_t duration_sec,
                                              uint32_t remaining_sec) {
  uint32_t clamped_remaining_sec;

  if (ctrl == NULL) {
    return;
  }

  clamped_remaining_sec = remaining_sec;
  if (clamped_remaining_sec > duration_sec) {
    clamped_remaining_sec = duration_sec;
  }

  ctrl->current_parcel.start_time =
      HAL_GetTick() - ((duration_sec - clamped_remaining_sec) * 1000UL);
}

uint32_t IRRIGATION_RUN_GetCurrentValveDurationSec(
    const irrigation_program_t *programs) {
  return PARCEL_SCHED_GetCurrentValveDurationSec(programs);
}

uint32_t IRRIGATION_RUN_GetCurrentValveTotalDurationSec(
    const irrigation_program_t *programs) {
  return IRRIGATION_RUN_GetCurrentPhaseDurationSec(programs,
                                                   PROGRAM_STATE_PRE_FLUSH) +
         IRRIGATION_RUN_GetCurrentValveDurationSec(programs) +
         IRRIGATION_RUN_GetCurrentPhaseDurationSec(programs,
                                                   PROGRAM_STATE_POST_FLUSH);
}

uint32_t IRRIGATION_RUN_GetCurrentPhaseDurationSec(
    const irrigation_program_t *programs,
    program_state_t phase) {
  uint8_t active_program_id = PARCEL_SCHED_GetActiveProgram();

  if (phase == PROGRAM_STATE_VALVE_ACTIVE) {
    return IRRIGATION_RUN_GetCurrentValveDurationSec(programs);
  }

  if (active_program_id == 0U || programs == NULL) {
    return 0U;
  }

  if (phase == PROGRAM_STATE_PRE_FLUSH) {
    return programs[active_program_id - 1U].pre_flush_sec;
  }

  if (phase == PROGRAM_STATE_POST_FLUSH) {
    return programs[active_program_id - 1U].post_flush_sec;
  }

  return 0U;
}

uint32_t IRRIGATION_RUN_GetCurrentWaitDurationSec(
    const irrigation_program_t *programs) {
  return PARCEL_SCHED_GetCurrentWaitDurationSec(programs);
}

uint32_t IRRIGATION_RUN_GetRemainingTime(
    const irrigation_control_t *ctrl,
    const irrigation_program_t *programs,
    program_state_t program_state) {
  uint32_t duration_sec;
  uint32_t elapsed_sec;
  uint32_t now;

  if (ctrl == NULL || programs == NULL || ctrl->is_running == 0U) {
    return 0U;
  }

  now = HAL_GetTick();
  if (program_state == PROGRAM_STATE_WAITING) {
    duration_sec = IRRIGATION_RUN_GetCurrentWaitDurationSec(programs);
    return PARCEL_SCHED_GetRemainingWaitSec(duration_sec);
  }

  if (ctrl->current_parcel.parcel_id == 0U) {
    return 0U;
  }

  duration_sec = ctrl->current_parcel.duration_sec;
  elapsed_sec = (now - ctrl->current_parcel.start_time) / 1000U;
  return (elapsed_sec >= duration_sec) ? 0U : (duration_sec - elapsed_sec);
}

irrigation_run_step_t IRRIGATION_RUN_ServiceValveTiming(
    irrigation_control_t *ctrl,
    const irrigation_program_t *programs,
    program_state_t phase,
    void (*on_parcel_complete)(uint8_t parcel_id)) {
  uint32_t elapsed_sec;

  if (ctrl == NULL || programs == NULL) {
    return IRRIGATION_RUN_STEP_NONE;
  }

  elapsed_sec = (HAL_GetTick() - ctrl->current_parcel.start_time) / 1000U;
  ctrl->current_parcel.elapsed_sec = elapsed_sec;
  if (elapsed_sec < ctrl->current_parcel.duration_sec) {
    return IRRIGATION_RUN_STEP_NONE;
  }

  if (phase == PROGRAM_STATE_PRE_FLUSH) {
    return IRRIGATION_RUN_STEP_PHASE_DONE;
  }

  if (phase == PROGRAM_STATE_VALVE_ACTIVE &&
      IRRIGATION_RUN_GetCurrentPhaseDurationSec(programs,
                                                PROGRAM_STATE_POST_FLUSH) > 0U) {
    return IRRIGATION_RUN_STEP_PHASE_DONE;
  }

  VALVES_Close(ctrl->current_parcel.valve_id);
  ctrl->total_parcel_count++;
  ctrl->total_watering_time +=
      IRRIGATION_RUN_GetCurrentValveTotalDurationSec(programs);
  ctrl->current_parcel.is_complete = 1U;
  if (on_parcel_complete != NULL) {
    on_parcel_complete(ctrl->current_parcel.parcel_id);
  }
  DOSING_CTRL_Reset();
  IRRIGATION_RUNTIME_RequestPersist();

  if (PARCEL_SCHED_HasMoreValvesInCycle() != 0U) {
    return IRRIGATION_RUN_STEP_NEXT_VALVE;
  }

  if (PARCEL_SCHED_HasMoreCycleRepeats(programs) != 0U &&
      IRRIGATION_RUN_GetCurrentWaitDurationSec(programs) > 0U) {
    PARCEL_SCHED_StartWaiting();
    return IRRIGATION_RUN_STEP_WAITING;
  }

  return IRRIGATION_RUN_STEP_NEXT_VALVE;
}

uint8_t IRRIGATION_RUN_ServiceWaiting(
    const irrigation_program_t *programs) {
  if (programs == NULL) {
    return 0U;
  }

  return PARCEL_SCHED_IsWaitingElapsed(
      IRRIGATION_RUN_GetCurrentWaitDurationSec(programs));
}
