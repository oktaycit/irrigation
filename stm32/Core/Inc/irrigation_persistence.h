/**
 ******************************************************************************
 * @file           : irrigation_persistence.h
 * @brief          : Sistem ayarlari ve program kaliciligi koordinatoru
 ******************************************************************************
 */

#ifndef __IRRIGATION_PERSISTENCE_H
#define __IRRIGATION_PERSISTENCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "eeprom.h"
#include <stdint.h>

void IRRIGATION_PERSIST_Init(void);
uint8_t IRRIGATION_PERSIST_LoadSystemRecord(eeprom_system_t *record);
void IRRIGATION_PERSIST_QueueSystemRecord(const eeprom_system_t *record);
uint8_t IRRIGATION_PERSIST_LoadPrograms(irrigation_program_t *programs,
                                        uint8_t count);
void IRRIGATION_PERSIST_QueueProgram(uint8_t program_id,
                                     const irrigation_program_t *program);
void IRRIGATION_PERSIST_Service(void);

#ifdef __cplusplus
}
#endif

#endif /* __IRRIGATION_PERSISTENCE_H */
