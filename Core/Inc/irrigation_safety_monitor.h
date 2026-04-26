/**
 ******************************************************************************
 * @file           : irrigation_safety_monitor.h
 * @brief          : Sulama guvenlik izleme ve hata siniflandirma yardimcilari
 ******************************************************************************
 */

#ifndef __IRRIGATION_SAFETY_MONITOR_H
#define __IRRIGATION_SAFETY_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "irrigation_types.h"
#include <stdint.h>

typedef enum {
  IRRIGATION_SAFETY_ACTION_NONE = 0,
  IRRIGATION_SAFETY_ACTION_ERROR,
  IRRIGATION_SAFETY_ACTION_EMERGENCY_STOP
} irrigation_safety_action_t;

typedef struct {
  uint8_t healthy;
  uint8_t valve_error_mask;
  irrigation_safety_action_t action;
  error_code_t error;
} irrigation_safety_result_t;

void IRRIGATION_SAFETY_Evaluate(uint8_t current_valve_id,
                                irrigation_safety_result_t *result);

#ifdef __cplusplus
}
#endif

#endif /* __IRRIGATION_SAFETY_MONITOR_H */
