/**
 ******************************************************************************
 * @file           : irrigation_schedule_trigger.h
 * @brief          : RTC tabanli program tetikleme ve zaman penceresi yardimcilari
 ******************************************************************************
 */

#ifndef __IRRIGATION_SCHEDULE_TRIGGER_H
#define __IRRIGATION_SCHEDULE_TRIGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "irrigation_types.h"
#include <stdint.h>

typedef struct {
  uint8_t rtc_ready;
  uint8_t weekday;
  uint16_t hhmm;
  uint16_t minute_of_day;
  uint16_t day_stamp;
} irrigation_schedule_context_t;

void IRRIGATION_TRIGGER_ReadContext(irrigation_schedule_context_t *context);
uint8_t IRRIGATION_TRIGGER_IsMinuteWithinWindow(uint16_t minute_of_day,
                                                uint16_t start_hhmm,
                                                uint16_t end_hhmm);
uint8_t IRRIGATION_TRIGGER_IsProgramDue(
    const irrigation_schedule_context_t *context,
    const irrigation_program_t *program);
uint8_t IRRIGATION_TRIGGER_SelectProgram(
    const irrigation_schedule_context_t *context,
    const irrigation_program_t *programs,
    uint8_t count,
    uint8_t *program_id);
uint8_t IRRIGATION_TRIGGER_IsPastEndWindow(
    const irrigation_schedule_context_t *context,
    uint16_t end_hhmm);

#ifdef __cplusplus
}
#endif

#endif /* __IRRIGATION_SCHEDULE_TRIGGER_H */
