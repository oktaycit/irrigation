/**
 ******************************************************************************
 * @file           : parcel_scheduler.c
 * @brief          : Parsel siralama ve queue runtime yonetimi
 ******************************************************************************
 */

#include "main.h"
#include "parcel_scheduler.h"
#include <string.h>

static parcel_scheduler_status_t g_scheduler = {0};

void PARCEL_SCHED_Init(void) { memset(&g_scheduler, 0, sizeof(g_scheduler)); }

void PARCEL_SCHED_Reset(void) { PARCEL_SCHED_Init(); }

void PARCEL_SCHED_ClearQueue(void) {
  memset(g_scheduler.queue, 0, sizeof(g_scheduler.queue));
  g_scheduler.queue_head = 0U;
  g_scheduler.queue_tail = 0U;
  g_scheduler.queue_size = 0U;
}

uint8_t PARCEL_SCHED_IsParcelQueued(uint8_t parcel_id) {
  uint8_t index = g_scheduler.queue_head;
  uint8_t remaining = g_scheduler.queue_size;

  while (remaining > 0U) {
    if (g_scheduler.queue[index] == parcel_id) {
      return 1U;
    }
    index = (index + 1U) % VALVE_COUNT;
    remaining--;
  }

  return 0U;
}

uint8_t PARCEL_SCHED_AddToQueue(uint8_t parcel_id) {
  if (parcel_id == 0U || parcel_id > VALVE_COUNT ||
      g_scheduler.queue_size >= VALVE_COUNT) {
    return 0U;
  }

  if (PARCEL_SCHED_IsParcelQueued(parcel_id) != 0U) {
    return 0U;
  }

  g_scheduler.queue[g_scheduler.queue_tail] = parcel_id;
  g_scheduler.queue_tail = (g_scheduler.queue_tail + 1U) % VALVE_COUNT;
  g_scheduler.queue_size++;
  return 1U;
}

uint8_t PARCEL_SCHED_GetQueueSize(void) { return g_scheduler.queue_size; }

uint8_t PARCEL_SCHED_BuildSequenceFromMask(uint8_t valve_mask) {
  g_scheduler.active_valve_count = 0U;
  memset(g_scheduler.active_valves, 0, sizeof(g_scheduler.active_valves));

  for (uint8_t i = 0U; i < VALVE_COUNT; i++) {
    if ((valve_mask & (1U << i)) == 0U) {
      continue;
    }
    g_scheduler.active_valves[g_scheduler.active_valve_count++] = (uint8_t)(i + 1U);
  }

  g_scheduler.active_valve_index = 0U;
  return (g_scheduler.active_valve_count > 0U) ? 1U : 0U;
}

uint8_t PARCEL_SCHED_BuildSequenceFromQueue(void) {
  uint8_t index = g_scheduler.queue_head;
  uint8_t count = g_scheduler.queue_size;

  g_scheduler.active_valve_count = 0U;
  memset(g_scheduler.active_valves, 0, sizeof(g_scheduler.active_valves));

  while (count > 0U) {
    uint8_t parcel_id = g_scheduler.queue[index];
    if (parcel_id != 0U) {
      g_scheduler.active_valves[g_scheduler.active_valve_count++] = parcel_id;
    }

    index = (index + 1U) % VALVE_COUNT;
    count--;
  }

  g_scheduler.active_valve_index = 0U;
  return (g_scheduler.active_valve_count > 0U) ? 1U : 0U;
}

void PARCEL_SCHED_SetManualSequence(uint8_t enabled) {
  g_scheduler.manual_sequence_active = (enabled != 0U) ? 1U : 0U;
}

uint8_t PARCEL_SCHED_IsManualSequence(void) {
  return g_scheduler.manual_sequence_active;
}

void PARCEL_SCHED_SetActiveProgram(uint8_t program_id) {
  g_scheduler.active_program_id = program_id;
}

uint8_t PARCEL_SCHED_GetActiveProgram(void) {
  return g_scheduler.active_program_id;
}

uint8_t PARCEL_SCHED_GetCurrentValve(void) {
  if (g_scheduler.active_valve_index >= g_scheduler.active_valve_count) {
    return 0U;
  }

  return g_scheduler.active_valves[g_scheduler.active_valve_index];
}

uint8_t PARCEL_SCHED_GetActiveValveCount(void) {
  return g_scheduler.active_valve_count;
}

uint8_t PARCEL_SCHED_GetActiveValveIndex(void) {
  return g_scheduler.active_valve_index;
}

void PARCEL_SCHED_SetActiveValveIndex(uint8_t index) {
  g_scheduler.active_valve_index = index;
}

uint8_t PARCEL_SCHED_GetRepeatIndex(void) { return g_scheduler.repeat_index; }

void PARCEL_SCHED_SetRepeatIndex(uint8_t index) {
  g_scheduler.repeat_index = index;
}

uint32_t PARCEL_SCHED_GetCurrentValveDurationSec(
    const irrigation_program_t *programs) {
  uint8_t valve_id = PARCEL_SCHED_GetCurrentValve();

  if (g_scheduler.manual_sequence_active != 0U) {
    return PARCELS_GetDuration(valve_id);
  }

  if (g_scheduler.active_program_id != 0U && programs != NULL) {
    const irrigation_program_t *program =
        &programs[g_scheduler.active_program_id - 1U];
    uint32_t duration = (uint32_t)program->irrigation_min * 60UL;

    if (duration == 0U && valve_id != 0U) {
      duration = PARCELS_GetDuration(valve_id);
    }

    return (duration == 0U) ? 60U : duration;
  }

  return 60U;
}

uint32_t PARCEL_SCHED_GetCurrentWaitDurationSec(
    const irrigation_program_t *programs) {
  if (g_scheduler.manual_sequence_active != 0U) {
    return 0U;
  }

  if (g_scheduler.active_program_id != 0U && programs != NULL) {
    return (uint32_t)programs[g_scheduler.active_program_id - 1U].wait_min * 60UL;
  }

  return 0U;
}

uint8_t PARCEL_SCHED_HasMoreValvesInCycle(void) {
  return ((g_scheduler.active_valve_index + 1U) < g_scheduler.active_valve_count)
             ? 1U
             : 0U;
}

uint8_t PARCEL_SCHED_HasMoreCycleRepeats(const irrigation_program_t *programs) {
  uint8_t repeat_total = 1U;

  if (g_scheduler.active_program_id != 0U && programs != NULL) {
    repeat_total = programs[g_scheduler.active_program_id - 1U].repeat_count;
    if (repeat_total == 0U) {
      repeat_total = 1U;
    }
  }

  return ((g_scheduler.repeat_index + 1U) < repeat_total) ? 1U : 0U;
}

uint8_t PARCEL_SCHED_Advance(const irrigation_program_t *programs) {
  uint8_t repeat_total = 1U;

  if (g_scheduler.active_program_id != 0U && programs != NULL) {
    repeat_total = programs[g_scheduler.active_program_id - 1U].repeat_count;
    if (repeat_total == 0U) {
      repeat_total = 1U;
    }
  }

  if ((g_scheduler.active_valve_index + 1U) < g_scheduler.active_valve_count) {
    g_scheduler.active_valve_index++;
    return 1U;
  }

  if ((g_scheduler.repeat_index + 1U) < repeat_total) {
    g_scheduler.repeat_index++;
    g_scheduler.active_valve_index = 0U;
    return 1U;
  }

  return 0U;
}

void PARCEL_SCHED_StartWaiting(void) {
  g_scheduler.wait_started_ms = HAL_GetTick();
}

void PARCEL_SCHED_SetWaitStartTick(uint32_t tick) {
  g_scheduler.wait_started_ms = tick;
}

uint32_t PARCEL_SCHED_GetWaitStartTick(void) {
  return g_scheduler.wait_started_ms;
}

uint32_t PARCEL_SCHED_GetRemainingWaitSec(uint32_t wait_duration_sec) {
  uint32_t elapsed_sec;

  if (wait_duration_sec == 0U) {
    return 0U;
  }

  elapsed_sec = (HAL_GetTick() - g_scheduler.wait_started_ms) / 1000U;
  return (elapsed_sec >= wait_duration_sec) ? 0U : (wait_duration_sec - elapsed_sec);
}

uint8_t PARCEL_SCHED_IsWaitingElapsed(uint32_t wait_duration_sec) {
  return (PARCEL_SCHED_GetRemainingWaitSec(wait_duration_sec) == 0U) ? 1U : 0U;
}

void PARCEL_SCHED_GetStatus(parcel_scheduler_status_t *status) {
  if (status == NULL) {
    return;
  }

  *status = g_scheduler;
}
