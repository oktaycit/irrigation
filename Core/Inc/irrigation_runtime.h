/**
 ******************************************************************************
 * @file           : irrigation_runtime.h
 * @brief          : Run lifecycle ve runtime backup koordinatoru
 ******************************************************************************
 */

#ifndef __IRRIGATION_RUNTIME_H
#define __IRRIGATION_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "irrigation_types.h"
#include <stdint.h>

void IRRIGATION_RUNTIME_Init(void);
void IRRIGATION_RUNTIME_Reset(void);
program_state_t IRRIGATION_RUNTIME_GetProgramState(void);
void IRRIGATION_RUNTIME_SetProgramState(program_state_t state);
void IRRIGATION_RUNTIME_RequestPersist(void);
uint8_t IRRIGATION_RUNTIME_NeedsPersist(void);
void IRRIGATION_RUNTIME_ServiceSnapshot(uint8_t is_running,
                                        uint32_t snapshot_period_ms);
uint8_t IRRIGATION_RUNTIME_LoadBackup(irrigation_runtime_backup_t *backup);
uint8_t IRRIGATION_RUNTIME_Persist(uint8_t is_running,
                                   const irrigation_runtime_backup_t *backup);

#ifdef __cplusplus
}
#endif

#endif /* __IRRIGATION_RUNTIME_H */
