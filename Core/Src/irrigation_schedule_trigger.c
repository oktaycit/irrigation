/**
 ******************************************************************************
 * @file           : irrigation_schedule_trigger.c
 * @brief          : RTC tabanli program tetikleme yardimcilari
 ******************************************************************************
 */

#include "irrigation_schedule_trigger.h"
#include "rtc_driver.h"
#include <string.h>

#define IRRIGATION_TRIGGER_DEFAULT_SUNRISE_MIN 360U
#define IRRIGATION_TRIGGER_DEFAULT_SUNSET_MIN 1080U
#define IRRIGATION_TRIGGER_MAX_DAILY_EVENTS 24U

static uint16_t IRRIGATION_TRIGGER_HHMMToMinuteOfDay(uint16_t hhmm);
static uint16_t IRRIGATION_TRIGGER_ApplyAnchorOffset(uint16_t anchor_minute,
                                                     int16_t offset_min);
static uint16_t IRRIGATION_TRIGGER_GetAnchorMinute(
    const irrigation_program_t *program);
static uint8_t IRRIGATION_TRIGGER_IsPeriodicProgramDue(
    const irrigation_schedule_context_t *context,
    const irrigation_program_t *program);
static uint8_t IRRIGATION_TRIGGER_IsDayEnabled(
    const irrigation_schedule_context_t *context, uint8_t days_mask);

void IRRIGATION_TRIGGER_ReadContext(irrigation_schedule_context_t *context) {
  rtc_time_t time = {0};
  rtc_date_t date = {0};

  if (context == NULL) {
    return;
  }

  memset(context, 0, sizeof(*context));
  if (RTC_IsInitialized() == 0U) {
    return;
  }

  RTC_GetTime(&time);
  RTC_GetDate(&date);

  context->rtc_ready = 1U;
  context->weekday = date.weekday;
  context->hhmm = (uint16_t)((time.hours * 100U) + time.minutes);
  context->minute_of_day = (uint16_t)((time.hours * 60U) + time.minutes);
  context->day_stamp =
      (uint16_t)((date.year * 512U) + (date.month * 32U) + date.date);
}

uint8_t IRRIGATION_TRIGGER_IsMinuteWithinWindow(uint16_t minute_of_day,
                                                uint16_t start_hhmm,
                                                uint16_t end_hhmm) {
  uint16_t start_minute = IRRIGATION_TRIGGER_HHMMToMinuteOfDay(start_hhmm);
  uint16_t end_minute = IRRIGATION_TRIGGER_HHMMToMinuteOfDay(end_hhmm);

  if (end_hhmm == 0U || start_minute == end_minute) {
    return (minute_of_day >= start_minute) ? 1U : 0U;
  }

  if (start_minute < end_minute) {
    return (minute_of_day >= start_minute && minute_of_day <= end_minute) ? 1U
                                                                           : 0U;
  }

  return (minute_of_day >= start_minute || minute_of_day <= end_minute) ? 1U
                                                                         : 0U;
}

uint8_t IRRIGATION_TRIGGER_IsProgramDue(
    const irrigation_schedule_context_t *context,
    const irrigation_program_t *program) {
  if (context == NULL || program == NULL || context->rtc_ready == 0U ||
      program->enabled != 1U) {
    return 0U;
  }

  if (IRRIGATION_TRIGGER_IsDayEnabled(context, program->days_mask) == 0U) {
    return 0U;
  }

  if (program->trigger_mode == IRRIGATION_TRIGGER_PERIODIC ||
      program->trigger_mode == IRRIGATION_TRIGGER_SUNRISE_PERIODIC) {
    return IRRIGATION_TRIGGER_IsPeriodicProgramDue(context, program);
  }

  if (IRRIGATION_TRIGGER_IsMinuteWithinWindow(context->minute_of_day,
                                              program->start_hhmm,
                                              program->end_hhmm) == 0U) {
    return 0U;
  }

  return (program->last_run_day != context->day_stamp) ? 1U : 0U;
}

uint8_t IRRIGATION_TRIGGER_SelectProgram(
    const irrigation_schedule_context_t *context,
    const irrigation_program_t *programs,
    uint8_t count,
    uint8_t *program_id) {
  if (program_id != NULL) {
    *program_id = 0U;
  }

  if (context == NULL || programs == NULL || count == 0U ||
      context->rtc_ready == 0U) {
    return 0U;
  }

  for (uint8_t i = 0U; i < count; i++) {
    if (IRRIGATION_TRIGGER_IsProgramDue(context, &programs[i]) == 0U) {
      continue;
    }

    if (program_id != NULL) {
      *program_id = i + 1U;
    }
    return 1U;
  }

  return 0U;
}

uint8_t IRRIGATION_TRIGGER_IsPastEndWindow(
    const irrigation_schedule_context_t *context,
    uint16_t end_hhmm) {
  if (context == NULL || context->rtc_ready == 0U || end_hhmm == 0U) {
    return 0U;
  }

  return (context->hhmm > end_hhmm) ? 1U : 0U;
}

static uint16_t IRRIGATION_TRIGGER_HHMMToMinuteOfDay(uint16_t hhmm) {
  uint16_t hours = (uint16_t)(hhmm / 100U);
  uint16_t minutes = (uint16_t)(hhmm % 100U);

  if (hours >= 24U || minutes >= 60U) {
    return 0U;
  }

  return (uint16_t)((hours * 60U) + minutes);
}

static uint16_t IRRIGATION_TRIGGER_ApplyAnchorOffset(uint16_t anchor_minute,
                                                     int16_t offset_min) {
  int32_t shifted = (int32_t)anchor_minute + (int32_t)offset_min;

  while (shifted < 0L) {
    shifted += 1440L;
  }
  while (shifted >= 1440L) {
    shifted -= 1440L;
  }

  return (uint16_t)shifted;
}

static uint16_t IRRIGATION_TRIGGER_GetAnchorMinute(
    const irrigation_program_t *program) {
  uint16_t anchor_minute;

  if (program == NULL) {
    return 0U;
  }

  if (program->trigger_mode == IRRIGATION_TRIGGER_SUNRISE_PERIODIC) {
    anchor_minute = IRRIGATION_TRIGGER_DEFAULT_SUNRISE_MIN;
  } else {
    anchor_minute = IRRIGATION_TRIGGER_HHMMToMinuteOfDay(program->start_hhmm);
  }

  return IRRIGATION_TRIGGER_ApplyAnchorOffset(anchor_minute,
                                              program->anchor_offset_min);
}

static uint8_t IRRIGATION_TRIGGER_IsPeriodicProgramDue(
    const irrigation_schedule_context_t *context,
    const irrigation_program_t *program) {
  uint16_t anchor_minute;
  uint16_t period_min;
  uint8_t max_events;
  uint16_t elapsed_min;
  uint16_t slot_index;
  uint16_t due_minute;

  if (context == NULL || program == NULL) {
    return 0U;
  }

  period_min = (program->period_min == 0U) ? 120U : program->period_min;
  max_events = (program->max_events_per_day == 0U) ? 1U
                                                   : program->max_events_per_day;
  if (max_events > IRRIGATION_TRIGGER_MAX_DAILY_EVENTS) {
    max_events = IRRIGATION_TRIGGER_MAX_DAILY_EVENTS;
  }

  anchor_minute = IRRIGATION_TRIGGER_GetAnchorMinute(program);
  if (context->minute_of_day < anchor_minute) {
    return 0U;
  }

  elapsed_min = (uint16_t)(context->minute_of_day - anchor_minute);
  slot_index = (uint16_t)((elapsed_min / period_min) + 1U);
  if (slot_index == 0U || slot_index > max_events) {
    return 0U;
  }

  due_minute = (uint16_t)(anchor_minute + ((slot_index - 1U) * period_min));
  if (due_minute >= 1440U) {
    return 0U;
  }

  if (program->last_run_day == context->day_stamp &&
      program->last_run_minute != 0xFFFFU &&
      program->last_run_minute >= due_minute) {
    return 0U;
  }

  return 1U;
}

static uint8_t IRRIGATION_TRIGGER_IsDayEnabled(
    const irrigation_schedule_context_t *context,
    uint8_t days_mask) {
  uint8_t bit;

  if (context == NULL || context->weekday < 1U || context->weekday > 7U) {
    return 0U;
  }

  bit = (uint8_t)(context->weekday - 1U);
  return ((days_mask & (1U << bit)) != 0U) ? 1U : 0U;
}
