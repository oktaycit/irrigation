/**
 ******************************************************************************
 * @file           : dosing_controller.c
 * @brief          : pH/EC dozaj durum makinesi
 ******************************************************************************
 */

#include "main.h"
#include "dosing_controller.h"
#include <math.h>
#include <string.h>

typedef struct {
  dosing_phase_t phase;
  dosing_phase_t mixing_source_phase;
  uint16_t active_mask;
  uint8_t pending_duty_percent;
  uint32_t dose_started_ms;
  uint32_t mix_started_ms;
  uint32_t settle_started_ms;
  control_state_t recommended_state;
  control_state_t resume_state;
  error_code_t last_error;
  uint8_t ph_correction_cycles;
  uint8_t ec_correction_cycles;
} dosing_runtime_t;

static dosing_runtime_t g_dosing = {0};
static const uint8_t g_ec_dosing_valve_ids[IRRIGATION_EC_CHANNEL_COUNT] = {
    DOSING_VALVE_FERT_A_ID, DOSING_VALVE_FERT_B_ID, DOSING_VALVE_FERT_C_ID,
    DOSING_VALVE_FERT_D_ID};

static uint8_t DOSING_CTRL_CalculateDemandDuty(float error, float hysteresis,
                                               uint8_t nominal_duty,
                                               uint8_t response_gain_percent);
static uint32_t DOSING_CTRL_GetFeedbackDelayMs(const ph_control_params_t *ph_params,
                                               const ec_control_params_t *ec_params);
static uint8_t DOSING_CTRL_IsCorrectionLimitReached(dosing_phase_t phase,
                                                    uint8_t max_cycles);
static void DOSING_CTRL_MarkCorrectionAttempt(dosing_phase_t phase);
static uint16_t DOSING_CTRL_ApplyPHDosingOutput(uint8_t valve_id,
                                                uint8_t duty_percent);
static uint16_t DOSING_CTRL_ApplyECDosingRecipe(const ec_control_params_t *ec_params,
                                                uint8_t total_duty);
static void DOSING_CTRL_StopActiveOutputs(void);
static void DOSING_CTRL_StartDose(const ph_control_params_t *ph_params,
                                  const ec_control_params_t *ec_params,
                                  uint8_t valve_id, dosing_phase_t phase,
                                  uint32_t duration_ms,
                                  control_state_t resume_state);
static void DOSING_CTRL_StopDose(void);

void DOSING_CTRL_Init(void) { memset(&g_dosing, 0, sizeof(g_dosing)); }

void DOSING_CTRL_Reset(void) {
  DOSING_CTRL_StopActiveOutputs();
  memset(&g_dosing, 0, sizeof(g_dosing));
  g_dosing.recommended_state = CTRL_STATE_PARCEL_WATERING;
  g_dosing.resume_state = CTRL_STATE_PARCEL_WATERING;
}

static uint8_t DOSING_CTRL_CalculateDemandDuty(float error, float hysteresis,
                                               uint8_t nominal_duty,
                                               uint8_t response_gain_percent) {
  float effective_hysteresis = hysteresis;
  float scaled_duty;
  uint8_t effective_gain = response_gain_percent;

  if (nominal_duty == 0U || error <= 0.0f) {
    return 0U;
  }

  if (effective_gain == 0U) {
    effective_gain = 100U;
  }

  if (effective_hysteresis < 0.05f) {
    effective_hysteresis = 0.05f;
  }

  scaled_duty = ((float)nominal_duty * error) / effective_hysteresis;
  if (scaled_duty < (float)nominal_duty) {
    scaled_duty = (float)nominal_duty;
  }
  if (scaled_duty > 100.0f) {
    scaled_duty = 100.0f;
  }
  scaled_duty = (scaled_duty * (float)effective_gain) / 100.0f;
  if (scaled_duty < 1.0f) {
    scaled_duty = 1.0f;
  }
  if (scaled_duty > 100.0f) {
    scaled_duty = 100.0f;
  }

  return (uint8_t)(scaled_duty + 0.5f);
}

static uint32_t DOSING_CTRL_GetFeedbackDelayMs(const ph_control_params_t *ph_params,
                                               const ec_control_params_t *ec_params) {
  uint32_t delay_ms = 0U;

  if (g_dosing.mixing_source_phase == DOSING_PHASE_EC && ec_params != NULL) {
    delay_ms = ec_params->feedback_delay_ms;
  } else if (ph_params != NULL) {
    delay_ms = ph_params->feedback_delay_ms;
  }

  if (delay_ms == 0U) {
    delay_ms = IRRIGATION_SETTLING_TIME * 1000U;
  }

  return delay_ms;
}

static uint8_t DOSING_CTRL_IsCorrectionLimitReached(dosing_phase_t phase,
                                                    uint8_t max_cycles) {
  if (max_cycles == 0U) {
    return 0U;
  }

  if (phase == DOSING_PHASE_PH) {
    return (g_dosing.ph_correction_cycles >= max_cycles) ? 1U : 0U;
  }

  if (phase == DOSING_PHASE_EC) {
    return (g_dosing.ec_correction_cycles >= max_cycles) ? 1U : 0U;
  }

  return 0U;
}

static void DOSING_CTRL_MarkCorrectionAttempt(dosing_phase_t phase) {
  if (phase == DOSING_PHASE_PH && g_dosing.ph_correction_cycles < 0xFFU) {
    g_dosing.ph_correction_cycles++;
  } else if (phase == DOSING_PHASE_EC &&
             g_dosing.ec_correction_cycles < 0xFFU) {
    g_dosing.ec_correction_cycles++;
  }
}

static uint16_t DOSING_CTRL_ApplyPHDosingOutput(uint8_t valve_id,
                                                uint8_t duty_percent) {
  if (valve_id == 0U || duty_percent == 0U) {
    return 0U;
  }

  VALVES_SetDosingDuty(valve_id, duty_percent);
  if (VALVES_RequestOpen(valve_id) == 0U) {
    g_dosing.last_error = ERR_VALVE_STUCK;
    return 0U;
  }

  return (uint16_t)(1U << (valve_id - 1U));
}

static uint16_t DOSING_CTRL_ApplyECDosingRecipe(const ec_control_params_t *ec_params,
                                                uint8_t total_duty) {
  uint16_t active_mask = 0U;
  uint16_t ratio_sum = 0U;

  if (ec_params == NULL) {
    g_dosing.last_error = ERR_VALVE_STUCK;
    return 0U;
  }

  for (uint8_t i = 0; i < IRRIGATION_EC_CHANNEL_COUNT; i++) {
    ratio_sum = (uint16_t)(ratio_sum + ec_params->recipe_ratio[i]);
  }

  if (ratio_sum == 0U || total_duty == 0U) {
    for (uint8_t i = 0; i < IRRIGATION_EC_CHANNEL_COUNT; i++) {
      VALVES_Close(g_ec_dosing_valve_ids[i]);
    }
    return 0U;
  }

  for (uint8_t i = 0; i < IRRIGATION_EC_CHANNEL_COUNT; i++) {
    uint8_t valve_id = g_ec_dosing_valve_ids[i];
    uint8_t valve_duty = 0U;

    if (ec_params->recipe_ratio[i] != 0U) {
      float share =
          ((float)ec_params->recipe_ratio[i] / (float)ratio_sum) * (float)total_duty;
      valve_duty = (uint8_t)(share + 0.5f);
    }

    if (valve_duty == 0U) {
      VALVES_Close(valve_id);
      continue;
    }

    VALVES_SetDosingDuty(valve_id, valve_duty);
    if (VALVES_RequestOpen(valve_id) == 0U) {
      g_dosing.last_error = ERR_VALVE_STUCK;
      return 0U;
    }

    active_mask |= (uint16_t)(1U << (valve_id - 1U));
  }

  return active_mask;
}

static void DOSING_CTRL_StopActiveOutputs(void) {
  for (uint8_t i = 0U; i < DOSING_VALVE_COUNT; i++) {
    VALVES_Close((uint8_t)(DOSING_VALVE_ACID_ID + i));
  }
  g_dosing.active_mask = 0U;
}

static void DOSING_CTRL_StartDose(const ph_control_params_t *ph_params,
                                  const ec_control_params_t *ec_params,
                                  uint8_t valve_id, dosing_phase_t phase,
                                  uint32_t duration_ms,
                                  control_state_t resume_state) {
  uint8_t duty_percent =
      (g_dosing.pending_duty_percent != 0U) ? g_dosing.pending_duty_percent
                                            : DOSING_PWM_DEFAULT_DUTY;

  (void)duration_ms;
  DOSING_CTRL_StopActiveOutputs();
  g_dosing.phase = phase;
  g_dosing.dose_started_ms = HAL_GetTick();
  g_dosing.recommended_state =
      (phase == DOSING_PHASE_PH) ? CTRL_STATE_PH_ADJUSTING : CTRL_STATE_EC_ADJUSTING;
  g_dosing.resume_state = resume_state;
  g_dosing.active_mask = 0U;
  g_dosing.last_error = ERR_NONE;

  if (phase == DOSING_PHASE_PH && valve_id != 0U) {
    g_dosing.active_mask = DOSING_CTRL_ApplyPHDosingOutput(valve_id, duty_percent);
  } else if (phase == DOSING_PHASE_EC && duty_percent != 0U) {
    g_dosing.active_mask = DOSING_CTRL_ApplyECDosingRecipe(ec_params, duty_percent);
  } else {
    g_dosing.phase = DOSING_PHASE_IDLE;
  }

  if (g_dosing.last_error != ERR_NONE || g_dosing.active_mask == 0U) {
    if (g_dosing.last_error == ERR_NONE) {
      g_dosing.phase = DOSING_PHASE_IDLE;
    }
    DOSING_CTRL_StopActiveOutputs();
  }

  g_dosing.pending_duty_percent = 0U;
  (void)ph_params;
}

void DOSING_CTRL_StartMixing(const ph_control_params_t *ph_params,
                             const ec_control_params_t *ec_params,
                             control_state_t resume_state) {
  (void)ph_params;
  (void)ec_params;
  g_dosing.mixing_source_phase =
      (g_dosing.phase == DOSING_PHASE_EC) ? DOSING_PHASE_EC : DOSING_PHASE_PH;
  g_dosing.phase = DOSING_PHASE_MIXING;
  g_dosing.mix_started_ms = HAL_GetTick();
  g_dosing.recommended_state = CTRL_STATE_MIXING;
  g_dosing.resume_state = resume_state;
}

void DOSING_CTRL_StopMixing(void) {
  if (g_dosing.phase == DOSING_PHASE_MIXING ||
      g_dosing.phase == DOSING_PHASE_SETTLING) {
    g_dosing.phase = DOSING_PHASE_IDLE;
    g_dosing.recommended_state = g_dosing.resume_state;
  }
}

uint8_t DOSING_CTRL_IsMixingComplete(const ph_control_params_t *ph_params,
                                     const ec_control_params_t *ec_params) {
  uint32_t wait_ms;

  wait_ms = (g_dosing.mixing_source_phase == DOSING_PHASE_EC)
                ? ec_params->wait_time_ms
                : ph_params->wait_time_ms;
  if (wait_ms == 0U) {
    wait_ms = IRRIGATION_MIXING_TIME * 1000U;
  }

  return ((HAL_GetTick() - g_dosing.mix_started_ms) >= wait_ms) ? 1U : 0U;
}

void DOSING_CTRL_StartSettling(control_state_t resume_state) {
  g_dosing.phase = DOSING_PHASE_SETTLING;
  g_dosing.settle_started_ms = HAL_GetTick();
  g_dosing.recommended_state = CTRL_STATE_SETTLING;
  g_dosing.resume_state = resume_state;
}

uint8_t DOSING_CTRL_IsSettlingComplete(const ph_control_params_t *ph_params,
                                       const ec_control_params_t *ec_params) {
  return ((HAL_GetTick() - g_dosing.settle_started_ms) >=
          DOSING_CTRL_GetFeedbackDelayMs(ph_params, ec_params))
             ? 1U
             : 0U;
}

void DOSING_CTRL_Update(const ph_control_params_t *ph_params,
                        const ec_control_params_t *ec_params,
                        const ph_sensor_data_t *ph_data,
                        const ec_sensor_data_t *ec_data,
                        control_state_t resume_state) {
  if (ph_params == NULL || ec_params == NULL || ph_data == NULL || ec_data == NULL) {
    g_dosing.last_error = ERR_COMMUNICATION;
    return;
  }

  g_dosing.resume_state = resume_state;
  g_dosing.last_error = ERR_NONE;

  switch (g_dosing.phase) {
  case DOSING_PHASE_IDLE:
    g_dosing.recommended_state = resume_state;

    if (ph_params->auto_adjust_enabled != 0U &&
        fabsf(ph_data->ph_value - ph_params->target) > ph_params->hysteresis) {
      float ph_error = fabsf(ph_data->ph_value - ph_params->target);
      if (DOSING_CTRL_IsCorrectionLimitReached(DOSING_PHASE_PH,
                                               ph_params->max_correction_cycles) !=
          0U) {
        g_dosing.last_error = ERR_TIMEOUT;
        return;
      }
      uint8_t ph_duty = DOSING_CTRL_CalculateDemandDuty(
          ph_error, ph_params->hysteresis, ph_params->pwm_duty_percent,
          ph_params->response_gain_percent);
      uint8_t valve_id = (ph_data->ph_value > ph_params->target)
                             ? ph_params->acid_valve_id
                             : ph_params->base_valve_id;

      g_dosing.pending_duty_percent = ph_duty;
      DOSING_CTRL_StartDose(ph_params, ec_params, valve_id, DOSING_PHASE_PH,
                            ph_params->dose_time_ms, resume_state);
      if (g_dosing.last_error != ERR_NONE) {
        return;
      }
      if (g_dosing.active_mask != 0U) {
        DOSING_CTRL_MarkCorrectionAttempt(DOSING_PHASE_PH);
      }
      if (valve_id == 0U) {
        DOSING_CTRL_StartMixing(ph_params, ec_params, resume_state);
      }
      return;
    }
    g_dosing.ph_correction_cycles = 0U;

    if (ec_params->auto_adjust_enabled != 0U &&
        ec_data->ec_value < (ec_params->target - ec_params->hysteresis)) {
      float ec_error = ec_params->target - ec_data->ec_value;
      if (DOSING_CTRL_IsCorrectionLimitReached(DOSING_PHASE_EC,
                                               ec_params->max_correction_cycles) !=
          0U) {
        g_dosing.last_error = ERR_TIMEOUT;
        return;
      }
      uint8_t ec_duty = DOSING_CTRL_CalculateDemandDuty(
          ec_error, ec_params->hysteresis, ec_params->pwm_duty_percent,
          ec_params->response_gain_percent);

      g_dosing.pending_duty_percent = ec_duty;
      DOSING_CTRL_StartDose(ph_params, ec_params, ec_duty, DOSING_PHASE_EC,
                            ec_params->dose_time_ms, resume_state);
      if (g_dosing.last_error != ERR_NONE) {
        return;
      }
      if (g_dosing.active_mask != 0U) {
        DOSING_CTRL_MarkCorrectionAttempt(DOSING_PHASE_EC);
      }
      if (ec_duty == 0U) {
        DOSING_CTRL_StartMixing(ph_params, ec_params, resume_state);
      }
    } else {
      g_dosing.ec_correction_cycles = 0U;
    }
    break;

  case DOSING_PHASE_PH:
    if ((HAL_GetTick() - g_dosing.dose_started_ms) >= ph_params->dose_time_ms) {
      DOSING_CTRL_StopDose();
      DOSING_CTRL_StartMixing(ph_params, ec_params, resume_state);
    }
    break;

  case DOSING_PHASE_EC:
    if ((HAL_GetTick() - g_dosing.dose_started_ms) >= ec_params->dose_time_ms) {
      DOSING_CTRL_StopDose();
      DOSING_CTRL_StartMixing(ph_params, ec_params, resume_state);
    }
    break;

  case DOSING_PHASE_MIXING:
    if (DOSING_CTRL_IsMixingComplete(ph_params, ec_params) != 0U) {
      DOSING_CTRL_StartSettling(resume_state);
    }
    break;

  case DOSING_PHASE_SETTLING:
    if (DOSING_CTRL_IsSettlingComplete(ph_params, ec_params) != 0U) {
      g_dosing.phase = DOSING_PHASE_IDLE;
      g_dosing.recommended_state = resume_state;
    }
    break;

  default:
    DOSING_CTRL_Reset();
    break;
  }
}

static void DOSING_CTRL_StopDose(void) {
  DOSING_CTRL_StopActiveOutputs();
  g_dosing.pending_duty_percent = 0U;
  g_dosing.phase = DOSING_PHASE_IDLE;
  g_dosing.dose_started_ms = 0U;
  g_dosing.mix_started_ms = 0U;
  g_dosing.settle_started_ms = 0U;
}

uint8_t DOSING_CTRL_IsBusy(void) {
  return (g_dosing.phase != DOSING_PHASE_IDLE) ? 1U : 0U;
}

uint16_t DOSING_CTRL_GetActiveMask(void) { return g_dosing.active_mask; }

error_code_t DOSING_CTRL_GetLastError(void) { return g_dosing.last_error; }

dosing_phase_t DOSING_CTRL_GetPhase(void) { return g_dosing.phase; }

control_state_t DOSING_CTRL_GetRecommendedState(void) {
  return g_dosing.recommended_state;
}

void DOSING_CTRL_GetStatus(dosing_controller_status_t *status) {
  if (status == NULL) {
    return;
  }

  status->phase = g_dosing.phase;
  status->active_mask = g_dosing.active_mask;
  status->pending_duty_percent = g_dosing.pending_duty_percent;
  status->recommended_state = g_dosing.recommended_state;
  status->last_error = g_dosing.last_error;
}
