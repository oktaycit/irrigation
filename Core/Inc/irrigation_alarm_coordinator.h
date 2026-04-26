/**
 ******************************************************************************
 * @file           : irrigation_alarm_coordinator.h
 * @brief          : Alarm/fault akisi icin orkestrasyon yardimcilari
 ******************************************************************************
 */

#ifndef __IRRIGATION_ALARM_COORDINATOR_H
#define __IRRIGATION_ALARM_COORDINATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "irrigation_control.h"

void IRRIGATION_ALARM_Sync(irrigation_control_t *ctrl);
void IRRIGATION_ALARM_RaiseError(irrigation_control_t *ctrl,
                                 error_code_t error,
                                 void (*change_state)(control_state_t),
                                 void (*on_error)(error_code_t));
void IRRIGATION_ALARM_Clear(irrigation_control_t *ctrl, error_code_t error);
void IRRIGATION_ALARM_ClearAll(irrigation_control_t *ctrl);
void IRRIGATION_ALARM_EnterEmergencyStop(
    irrigation_control_t *ctrl,
    error_code_t fallback_error,
    void (*change_state)(control_state_t));
uint8_t IRRIGATION_ALARM_Acknowledge(irrigation_control_t *ctrl);
uint8_t IRRIGATION_ALARM_Reset(
    irrigation_control_t *ctrl,
    void (*stop_handler)(void),
    uint8_t (*safety_check_handler)(void));

#ifdef __cplusplus
}
#endif

#endif /* __IRRIGATION_ALARM_COORDINATOR_H */
