/**
 ******************************************************************************
 * @file           : parcel_scheduler.h
 * @brief          : Parsel siralama ve queue runtime yonetimi
 ******************************************************************************
 */

#ifndef __PARCEL_SCHEDULER_H
#define __PARCEL_SCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "irrigation_types.h"
#include "valves.h"

typedef struct {
  uint8_t queue[VALVE_COUNT];
  uint8_t queue_head;
  uint8_t queue_tail;
  uint8_t queue_size;
  uint8_t active_program_id;
  uint8_t manual_sequence_active;
  uint8_t active_valves[VALVE_COUNT];
  uint8_t active_valve_count;
  uint8_t active_valve_index;
  uint8_t repeat_index;
  uint32_t wait_started_ms;
} parcel_scheduler_status_t;

void PARCEL_SCHED_Init(void);
void PARCEL_SCHED_Reset(void);
void PARCEL_SCHED_ClearQueue(void);
uint8_t PARCEL_SCHED_AddToQueue(uint8_t parcel_id);
uint8_t PARCEL_SCHED_IsParcelQueued(uint8_t parcel_id);
uint8_t PARCEL_SCHED_GetQueueSize(void);
uint8_t PARCEL_SCHED_BuildSequenceFromMask(uint8_t valve_mask);
uint8_t PARCEL_SCHED_BuildSequenceFromQueue(void);
void PARCEL_SCHED_SetManualSequence(uint8_t enabled);
uint8_t PARCEL_SCHED_IsManualSequence(void);
void PARCEL_SCHED_SetActiveProgram(uint8_t program_id);
uint8_t PARCEL_SCHED_GetActiveProgram(void);
uint8_t PARCEL_SCHED_GetCurrentValve(void);
uint8_t PARCEL_SCHED_GetActiveValveCount(void);
uint8_t PARCEL_SCHED_GetActiveValveIndex(void);
void PARCEL_SCHED_SetActiveValveIndex(uint8_t index);
uint8_t PARCEL_SCHED_GetRepeatIndex(void);
void PARCEL_SCHED_SetRepeatIndex(uint8_t index);
uint32_t PARCEL_SCHED_GetCurrentValveDurationSec(
    const irrigation_program_t *programs);
uint32_t PARCEL_SCHED_GetCurrentWaitDurationSec(
    const irrigation_program_t *programs);
uint8_t PARCEL_SCHED_HasMoreValvesInCycle(void);
uint8_t PARCEL_SCHED_HasMoreCycleRepeats(const irrigation_program_t *programs);
uint8_t PARCEL_SCHED_Advance(const irrigation_program_t *programs);
void PARCEL_SCHED_StartWaiting(void);
void PARCEL_SCHED_SetWaitStartTick(uint32_t tick);
uint32_t PARCEL_SCHED_GetWaitStartTick(void);
uint32_t PARCEL_SCHED_GetRemainingWaitSec(uint32_t wait_duration_sec);
uint8_t PARCEL_SCHED_IsWaitingElapsed(uint32_t wait_duration_sec);
void PARCEL_SCHED_GetStatus(parcel_scheduler_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* __PARCEL_SCHEDULER_H */
