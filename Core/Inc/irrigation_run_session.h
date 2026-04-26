/**
 ******************************************************************************
 * @file           : irrigation_run_session.h
 * @brief          : Aktif sulama oturumu ve parsel runtime yardimcilari
 ******************************************************************************
 */

#ifndef __IRRIGATION_RUN_SESSION_H
#define __IRRIGATION_RUN_SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "irrigation_control.h"

typedef enum {
  IRRIGATION_RUN_STEP_NONE = 0,
  IRRIGATION_RUN_STEP_PHASE_DONE,
  IRRIGATION_RUN_STEP_NEXT_VALVE,
  IRRIGATION_RUN_STEP_WAITING
} irrigation_run_step_t;

void IRRIGATION_RUN_Begin(irrigation_control_t *ctrl);
void IRRIGATION_RUN_ApplyProgramTargets(irrigation_control_t *ctrl,
                                        const irrigation_program_t *program);
void IRRIGATION_RUN_Stop(irrigation_control_t *ctrl);
void IRRIGATION_RUN_Pause(irrigation_control_t *ctrl);
void IRRIGATION_RUN_Resume(irrigation_control_t *ctrl);
void IRRIGATION_RUN_ResetCurrentParcel(irrigation_control_t *ctrl);
uint8_t IRRIGATION_RUN_StartCurrentValve(irrigation_control_t *ctrl,
                                         const irrigation_program_t *programs,
                                         program_state_t phase);
uint8_t IRRIGATION_RUN_ActivateCurrentValve(irrigation_control_t *ctrl,
                                            const irrigation_program_t *programs,
                                            program_state_t phase);
void IRRIGATION_RUN_StartPhase(irrigation_control_t *ctrl,
                               const irrigation_program_t *programs,
                               program_state_t phase);
void IRRIGATION_RUN_RestoreCurrentValveTiming(irrigation_control_t *ctrl,
                                              uint32_t duration_sec,
                                              uint32_t remaining_sec);
uint32_t IRRIGATION_RUN_GetCurrentValveDurationSec(
    const irrigation_program_t *programs);
uint32_t IRRIGATION_RUN_GetCurrentValveTotalDurationSec(
    const irrigation_program_t *programs);
uint32_t IRRIGATION_RUN_GetCurrentPhaseDurationSec(
    const irrigation_program_t *programs,
    program_state_t phase);
uint32_t IRRIGATION_RUN_GetCurrentWaitDurationSec(
    const irrigation_program_t *programs);
uint32_t IRRIGATION_RUN_GetRemainingTime(
    const irrigation_control_t *ctrl,
    const irrigation_program_t *programs,
    program_state_t program_state);
irrigation_run_step_t IRRIGATION_RUN_ServiceValveTiming(
    irrigation_control_t *ctrl,
    const irrigation_program_t *programs,
    program_state_t phase,
    void (*on_parcel_complete)(uint8_t parcel_id));
uint8_t IRRIGATION_RUN_ServiceWaiting(
    const irrigation_program_t *programs);

#ifdef __cplusplus
}
#endif

#endif /* __IRRIGATION_RUN_SESSION_H */
