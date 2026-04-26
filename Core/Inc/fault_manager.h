/**
 ******************************************************************************
 * @file           : fault_manager.h
 * @brief          : Merkezilestirilmis hata ve alarm yonetimi
 ******************************************************************************
 */

#ifndef __FAULT_MANAGER_H
#define __FAULT_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "irrigation_types.h"
#include <stdint.h>

typedef struct {
  uint8_t active;
  uint8_t latched;
  uint8_t manual_ack_required;
  uint8_t acknowledged;
  uint8_t valve_error_mask;
  error_code_t last_error;
  control_state_t recommended_state;
  char alarm_text[32];
  char detail_text[96];
  char action_text[64];
} fault_manager_status_t;

void FAULT_MGR_Init(void);
void FAULT_MGR_Reset(void);
void FAULT_MGR_Raise(error_code_t error, uint8_t latched,
                     uint8_t manual_ack_required,
                     control_state_t recommended_state);
void FAULT_MGR_Clear(error_code_t error);
void FAULT_MGR_ClearAll(void);
void FAULT_MGR_EnterEmergencyStop(error_code_t fallback_error);
void FAULT_MGR_Acknowledge(void);
void FAULT_MGR_SetAlarmText(const char *text);
void FAULT_MGR_SetValveErrorMask(uint8_t valve_error_mask);
uint8_t FAULT_MGR_GetValveErrorMask(void);
uint8_t FAULT_MGR_HasActiveFault(void);
uint8_t FAULT_MGR_IsLatched(void);
uint8_t FAULT_MGR_RequiresManualAck(void);
uint8_t FAULT_MGR_IsAcknowledged(void);
uint8_t FAULT_MGR_CanReset(void);
error_code_t FAULT_MGR_GetLastError(void);
control_state_t FAULT_MGR_GetRecommendedState(void);
const char *FAULT_MGR_GetAlarmText(void);
const char *FAULT_MGR_GetDetailText(void);
const char *FAULT_MGR_GetActionText(void);
const char *FAULT_MGR_GetErrorString(error_code_t error);
void FAULT_MGR_GetStatus(fault_manager_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* __FAULT_MANAGER_H */
