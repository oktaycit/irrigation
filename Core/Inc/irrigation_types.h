/**
 ******************************************************************************
 * @file           : irrigation_types.h
 * @brief          : Sulama kontrolu tarafinda paylasilan ortak tipler
 ******************************************************************************
 */

#ifndef __IRRIGATION_TYPES_H
#define __IRRIGATION_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define IRRIGATION_EC_CHANNEL_COUNT 4U
#define IRRIGATION_PROGRAM_COUNT 8U
#define IRRIGATION_PROGRAM_VALVE_COUNT 8U
#define IRRIGATION_DEFAULT_PRE_FLUSH_SEC 60U
#define IRRIGATION_DEFAULT_POST_FLUSH_SEC 120U
#define IRRIGATION_MAX_FLUSH_SEC 900U

typedef enum {
  CTRL_STATE_IDLE = 0,
  CTRL_STATE_CHECKING_SENSORS,
  CTRL_STATE_PH_ADJUSTING,
  CTRL_STATE_EC_ADJUSTING,
  CTRL_STATE_MIXING,
  CTRL_STATE_SETTLING,
  CTRL_STATE_PRE_FLUSH,
  CTRL_STATE_PARCEL_WATERING,
  CTRL_STATE_POST_FLUSH,
  CTRL_STATE_WAITING,
  CTRL_STATE_ERROR,
  CTRL_STATE_EMERGENCY_STOP
} control_state_t;

typedef enum {
  PROGRAM_STATE_IDLE = 0,
  PROGRAM_STATE_VALVE_ACTIVE,
  PROGRAM_STATE_WAITING,
  PROGRAM_STATE_NEXT_VALVE,
  PROGRAM_STATE_ERROR,
  PROGRAM_STATE_PRE_FLUSH,
  PROGRAM_STATE_POST_FLUSH
} program_state_t;

typedef enum {
  ERR_NONE = 0,
  ERR_PH_SENSOR_FAULT,
  ERR_EC_SENSOR_FAULT,
  ERR_PH_OUT_OF_RANGE,
  ERR_EC_OUT_OF_RANGE,
  ERR_VALVE_STUCK,
  ERR_TIMEOUT,
  ERR_LOW_WATER_LEVEL,
  ERR_OVERCURRENT,
  ERR_COMMUNICATION,
  ERR_MAX
} error_code_t;

typedef struct {
  float target;
  float min_limit;
  float max_limit;
  float hysteresis;
  uint8_t acid_valve_id;
  uint8_t base_valve_id;
  uint32_t dose_time_ms;
  uint32_t wait_time_ms;
  uint8_t auto_adjust_enabled;
  uint8_t pwm_duty_percent; /* Generic 0-100 dosing demand for valve PWM. */
  uint32_t feedback_delay_ms;      /* Extra wait before trusting sensor feedback. */
  uint8_t response_gain_percent;   /* 100=nominal, lower values soften dosing. */
  uint8_t max_correction_cycles;   /* 0=unlimited consecutive correction cycles. */
  uint8_t fertilizer_select;
} ph_control_params_t;

typedef struct {
  float target;
  float min_limit;
  float max_limit;
  float hysteresis;
  uint8_t fertilizer_valve_id;
  uint32_t dose_time_ms;
  uint32_t wait_time_ms;
  uint8_t auto_adjust_enabled;
  uint8_t pwm_duty_percent; /* Generic 0-100 dosing demand for valve PWM. */
  uint32_t feedback_delay_ms;      /* Extra wait before trusting sensor feedback. */
  uint8_t response_gain_percent;   /* 100=nominal, lower values soften dosing. */
  uint8_t max_correction_cycles;   /* 0=unlimited consecutive correction cycles. */
  uint8_t fertilizer_select;
  uint8_t recipe_ratio[IRRIGATION_EC_CHANNEL_COUNT];
} ec_control_params_t;

typedef struct {
  uint8_t parcel_id;
  uint8_t valve_id;
  uint32_t start_time;
  uint32_t duration_sec;
  uint32_t elapsed_sec;
  uint8_t is_complete;
  uint8_t is_skipped;
} parcel_watering_t;

typedef struct {
  uint32_t system_start_time;
  uint32_t last_ph_check;
  uint32_t last_ec_check;
  uint32_t last_irrigation;
  uint32_t state_entry_time;
  uint32_t dose_start_time;
  uint32_t mixing_start_time;
  uint32_t mixing_duration_ms;
  uint32_t settling_start_time;
  uint32_t settling_duration_ms;
} irrigation_timers_t;

typedef struct {
  uint8_t enabled;
  uint8_t hour;
  uint8_t minute;
  uint8_t days_of_week;
  uint8_t parcel_id;
} irrigation_schedule_t;

typedef struct {
  uint8_t enabled;
  uint16_t start_hhmm;
  uint16_t end_hhmm;
  uint8_t valve_mask;
  uint16_t irrigation_min;
  uint16_t wait_min;
  uint8_t repeat_count;
  uint8_t days_mask;
  int16_t ec_set_x100;
  int16_t ph_set_x100;
  uint8_t fert_ratio_percent[IRRIGATION_EC_CHANNEL_COUNT];
  uint16_t pre_flush_sec;
  uint16_t post_flush_sec;
  uint16_t last_run_day;
  uint16_t last_run_minute;
  uint16_t crc;
} irrigation_program_t;

typedef struct {
  uint8_t valid;
  uint8_t active_program_id;
  uint8_t active_valve_index;
  uint8_t repeat_index;
  uint8_t program_state;
  uint8_t active_valve_id;
  uint16_t remaining_sec;
  int16_t ec_target_x100;
  int16_t ph_target_x100;
  uint8_t error_code;
  uint8_t reserved[5];
  uint16_t crc;
} irrigation_runtime_backup_t;

#ifdef __cplusplus
}
#endif

#endif /* __IRRIGATION_TYPES_H */
