/**
 ******************************************************************************
 * @file           : irrigation_runtime.c
 * @brief          : Run lifecycle ve runtime backup koordinatoru
 ******************************************************************************
 */

#include "irrigation_runtime.h"
#include "eeprom.h"
#include "stm32f4xx_hal.h"
#include <string.h>

typedef struct {
  program_state_t program_state;
  uint8_t persist_dirty;
  uint32_t last_snapshot_ms;
} irrigation_runtime_state_t;

static irrigation_runtime_state_t g_runtime = {0};

void IRRIGATION_RUNTIME_Init(void) {
  memset(&g_runtime, 0, sizeof(g_runtime));
  g_runtime.program_state = PROGRAM_STATE_IDLE;
}

void IRRIGATION_RUNTIME_Reset(void) { IRRIGATION_RUNTIME_Init(); }

program_state_t IRRIGATION_RUNTIME_GetProgramState(void) {
  return g_runtime.program_state;
}

void IRRIGATION_RUNTIME_SetProgramState(program_state_t state) {
  g_runtime.program_state = state;
  g_runtime.persist_dirty = 1U;
}

void IRRIGATION_RUNTIME_RequestPersist(void) { g_runtime.persist_dirty = 1U; }

uint8_t IRRIGATION_RUNTIME_NeedsPersist(void) { return g_runtime.persist_dirty; }

void IRRIGATION_RUNTIME_ServiceSnapshot(uint8_t is_running,
                                        uint32_t snapshot_period_ms) {
  uint32_t now;

  if (is_running == 0U || snapshot_period_ms == 0U) {
    return;
  }

  now = HAL_GetTick();
  if ((now - g_runtime.last_snapshot_ms) < snapshot_period_ms) {
    return;
  }

  g_runtime.last_snapshot_ms = now;
  g_runtime.persist_dirty = 1U;
}

uint8_t IRRIGATION_RUNTIME_LoadBackup(irrigation_runtime_backup_t *backup) {
  if (backup == NULL) {
    return EEPROM_READ_ERROR;
  }

  return EEPROM_LoadRuntimeBackup(backup);
}

uint8_t IRRIGATION_RUNTIME_Persist(uint8_t is_running,
                                   const irrigation_runtime_backup_t *backup) {
  uint8_t result;

  if (backup == NULL) {
    return EEPROM_WRITE_ERROR;
  }

  if (g_runtime.persist_dirty == 0U) {
    return EEPROM_OK;
  }

  if (is_running != 0U) {
    result = EEPROM_SaveRuntimeBackup(backup);
  } else {
    result = EEPROM_ClearRuntimeBackup();
  }

  if (result == EEPROM_OK) {
    g_runtime.persist_dirty = 0U;
  }

  return result;
}
