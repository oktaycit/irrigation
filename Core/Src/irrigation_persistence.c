/**
 ******************************************************************************
 * @file           : irrigation_persistence.c
 * @brief          : Sistem ayarlari ve program kaliciligi koordinatoru
 ******************************************************************************
 */

#include "irrigation_persistence.h"
#include "valves.h"
#include <string.h>

typedef struct {
  eeprom_system_t system_record;
  irrigation_program_t programs[IRRIGATION_PROGRAM_COUNT];
  uint8_t system_dirty;
  uint8_t program_dirty_mask;
} irrigation_persistence_state_t;

static irrigation_persistence_state_t g_persist = {0};

void IRRIGATION_PERSIST_Init(void) { memset(&g_persist, 0, sizeof(g_persist)); }

uint8_t IRRIGATION_PERSIST_LoadSystemRecord(eeprom_system_t *record) {
  uint8_t result;

  result = EEPROM_LoadSystemParams();
  (void)EEPROM_GetSystemParams(&g_persist.system_record);

  if (record != NULL) {
    *record = g_persist.system_record;
  }

  return result;
}

void IRRIGATION_PERSIST_QueueSystemRecord(const eeprom_system_t *record) {
  if (record == NULL) {
    return;
  }

  g_persist.system_record = *record;
  g_persist.system_dirty = 1U;
}

uint8_t IRRIGATION_PERSIST_LoadPrograms(irrigation_program_t *programs,
                                        uint8_t count) {
  uint8_t result;
  uint8_t max_count;

  if (programs == NULL) {
    return EEPROM_ERROR;
  }

  max_count = (count > IRRIGATION_PROGRAM_COUNT) ? IRRIGATION_PROGRAM_COUNT : count;
  result = EEPROM_LoadAllPrograms(g_persist.programs, max_count);
  memcpy(programs, g_persist.programs,
         (size_t)max_count * sizeof(irrigation_program_t));
  return result;
}

void IRRIGATION_PERSIST_QueueProgram(uint8_t program_id,
                                     const irrigation_program_t *program) {
  if (program_id == 0U || program_id > IRRIGATION_PROGRAM_COUNT || program == NULL) {
    return;
  }

  g_persist.programs[program_id - 1U] = *program;
  g_persist.program_dirty_mask |= (uint8_t)(1U << (program_id - 1U));
}

void IRRIGATION_PERSIST_Service(void) {
  if (g_persist.system_dirty != 0U) {
    if (EEPROM_SetSystemParams(&g_persist.system_record) == EEPROM_OK) {
      g_persist.system_dirty = 0U;
    }
  }

  for (uint8_t i = 0U; i < IRRIGATION_PROGRAM_COUNT; i++) {
    uint8_t bit = (uint8_t)(1U << i);

    if ((g_persist.program_dirty_mask & bit) == 0U) {
      continue;
    }

    if (EEPROM_SaveProgram(i + 1U, &g_persist.programs[i]) == EEPROM_OK) {
      g_persist.program_dirty_mask &= (uint8_t)~bit;
    }
  }
}
