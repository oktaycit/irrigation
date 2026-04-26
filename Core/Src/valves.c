/**
 ******************************************************************************
 * @file           : valves.c
 * @brief          : Vana Kontrol Driver (PCF8574 I2C Expander ile)
 ******************************************************************************
 */

#include "main.h"
#include "valves.h"
#include "pcf8574.h"
#include <string.h>
#include <stdio.h>

/* External I2C Handle */
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim4;

/* SYSTEM_Log fonksiyonları yoksa define et */
#ifndef SYSTEM_LogInfo
#define SYSTEM_LogInfo(...)  ((void)0)
#endif
#ifndef SYSTEM_LogError
#define SYSTEM_LogError(...) ((void)0)
#endif
#ifndef SYSTEM_LogWarn
#define SYSTEM_LogWarn(...)  ((void)0)
#endif
#ifndef GUI_ShowError
#define GUI_ShowError(...)   ((void)0)
#endif

/* Global Valve Data --------------------------------------------------------*/
static valve_t g_valves[TOTAL_VALVE_COUNT] = {0};
static parcel_t g_parcels[PARCEL_VALVE_COUNT] = {0};
typedef struct {
  GPIO_TypeDef *port;
  uint16_t pin;
  TIM_HandleTypeDef *pwm_timer;
  uint32_t pwm_channel;
  uint8_t hardware_pwm;
  uint8_t frequency_hz;
  uint8_t min_duty_percent;
  uint8_t max_duty_percent;
  uint8_t target_duty_percent;
  uint8_t applied_duty_percent;
  uint8_t ramp_step_percent;
  uint8_t enabled;
  uint8_t output_state;
  uint16_t period_ticks;
  uint16_t on_ticks;
  uint16_t tick_in_period;
} dosing_pwm_channel_t;

static dosing_pwm_channel_t g_dosing_pwm[DOSING_VALVE_COUNT] = {
    {
        .port = DOSING_ACID_PORT,
        .pin = DOSING_ACID_PIN,
        .pwm_timer = &htim4,
        .pwm_channel = TIM_CHANNEL_1,
        .hardware_pwm = 1U,
        .frequency_hz = DOSING_HW_PWM_FREQ_HZ,
        .min_duty_percent = DOSING_PWM_DEFAULT_MIN_DUTY,
        .max_duty_percent = DOSING_PWM_DEFAULT_MAX_DUTY,
        .target_duty_percent = DOSING_PWM_DEFAULT_DUTY,
        .ramp_step_percent = DOSING_PWM_DEFAULT_RAMP_STEP,
        .period_ticks = 40U,
    },
    {
        .port = DOSING_FERT_A_PORT,
        .pin = DOSING_FERT_A_PIN,
        .pwm_timer = &htim4,
        .pwm_channel = TIM_CHANNEL_2,
        .hardware_pwm = 1U,
        .frequency_hz = DOSING_HW_PWM_FREQ_HZ,
        .min_duty_percent = DOSING_PWM_DEFAULT_MIN_DUTY,
        .max_duty_percent = DOSING_PWM_DEFAULT_MAX_DUTY,
        .target_duty_percent = DOSING_PWM_DEFAULT_DUTY,
        .ramp_step_percent = DOSING_PWM_DEFAULT_RAMP_STEP,
        .period_ticks = 40U,
    },
    {
        .port = DOSING_FERT_B_PORT,
        .pin = DOSING_FERT_B_PIN,
        .frequency_hz = DOSING_FERT_PWM_DEFAULT_FREQ_HZ,
        .min_duty_percent = DOSING_PWM_DEFAULT_MIN_DUTY,
        .max_duty_percent = DOSING_PWM_DEFAULT_MAX_DUTY,
        .target_duty_percent = DOSING_PWM_DEFAULT_DUTY,
        .ramp_step_percent = DOSING_PWM_DEFAULT_RAMP_STEP,
        .period_ticks = 40U,
    },
    {
        .port = DOSING_FERT_C_PORT,
        .pin = DOSING_FERT_C_PIN,
        .frequency_hz = DOSING_FERT_PWM_DEFAULT_FREQ_HZ,
        .min_duty_percent = DOSING_PWM_DEFAULT_MIN_DUTY,
        .max_duty_percent = DOSING_PWM_DEFAULT_MAX_DUTY,
        .target_duty_percent = DOSING_PWM_DEFAULT_DUTY,
        .ramp_step_percent = DOSING_PWM_DEFAULT_RAMP_STEP,
        .period_ticks = 40U,
    },
    {
        .port = DOSING_FERT_D_PORT,
        .pin = DOSING_FERT_D_PIN,
        .frequency_hz = DOSING_FERT_PWM_DEFAULT_FREQ_HZ,
        .min_duty_percent = DOSING_PWM_DEFAULT_MIN_DUTY,
        .max_duty_percent = DOSING_PWM_DEFAULT_MAX_DUTY,
        .target_duty_percent = DOSING_PWM_DEFAULT_DUTY,
        .ramp_step_percent = DOSING_PWM_DEFAULT_RAMP_STEP,
        .period_ticks = 40U,
    },
};

/* Safety & Validation ------------------------------------------------------*/
static uint16_t g_valve_command_crc = 0U;
static uint32_t g_last_command_time = 0U;
static uint8_t g_consecutive_faults = 0U;
static uint8_t g_expected_state[TOTAL_VALVE_COUNT] = {0};
static uint8_t g_error_mask = 0U;

/* Private Function Prototypes ----------------------------------------------*/
static void VALVES_UpdateState(uint8_t id, valve_state_t new_state);
static uint8_t VALVES_IsValidValveId(uint8_t valve_id);
static uint8_t VALVES_GetPCFChip(uint8_t valve_id);
static uint8_t VALVES_GetPCFPin(uint8_t valve_id);
static uint16_t VALVES_CalculateCommandCRC(void);
static int8_t VALVES_GetDosingIndex(uint8_t valve_id);
static void VALVES_WriteDosingOutput(uint8_t dosing_index, GPIO_PinState state);
static void VALVES_ApplyHardwareDosingPWM(uint8_t dosing_index);
static uint8_t VALVES_ClampDosingDuty(const dosing_pwm_channel_t *channel,
                                      uint8_t duty_percent);
static void VALVES_RefreshDosingTiming(uint8_t dosing_index);
static void VALVES_StepDosingRamp(uint8_t dosing_index);
static void VALVES_RecordFault(uint8_t valve_id, uint8_t fault_flag);
static void VALVES_ClearFault(uint8_t valve_id, uint8_t fault_flag);
static void VALVES_ClearAllValveFaults(uint8_t fault_flag);
static void VALVES_CloseOtherParcelValves(uint8_t keep_open_valve_id);
static void VALVES_ApplyDosingInterlock(uint8_t valve_id);
static uint8_t VALVES_ShouldEmulateParcelOutputs(void);
static void VALVES_MarkParcelValveOpened(uint8_t valve_id);
static void VALVES_MarkParcelValveClosed(uint8_t valve_id, uint32_t now);

/**
 * @brief  Initialize Valves System with PCF8574 (Parcel) + Direct GPIO (Dosing)
 */
void VALVES_Init(void) {
  /* Initialize PCF8574 chips - ONLY for parcel valves */
  pcf8574_status_t status = PCF8574_InitAll(&hi2c1, 1U);  /* 1 chip: parcel only */

  if (status != PCF8574_OK) {
    SYSTEM_LogError("PCF8574 initialization failed!");
  }

  /* Initialize valve structures */
  memset(g_valves, 0, sizeof(g_valves));
  memset(g_parcels, 0, sizeof(g_parcels));
  memset(g_expected_state, 0, sizeof(g_expected_state));
  g_error_mask = 0U;
  g_consecutive_faults = 0U;
  g_last_command_time = 0U;
  for (uint8_t i = 0; i < DOSING_VALVE_COUNT; i++) {
    g_dosing_pwm[i].target_duty_percent = DOSING_PWM_DEFAULT_DUTY;
    g_dosing_pwm[i].applied_duty_percent = 0U;
    g_dosing_pwm[i].enabled = 0U;
    g_dosing_pwm[i].output_state = 0U;
    g_dosing_pwm[i].tick_in_period = 0U;
    VALVES_RefreshDosingTiming(i);
  }

  /* Parsel Vanaları Initialize (1-8) - PCF8574 */
  for (uint8_t i = 0; i < PARCEL_VALVE_COUNT; i++) {
    g_valves[i].id = i + 1;
    g_valves[i].state = VALVE_STATE_CLOSED;
    g_valves[i].mode = VALVE_MODE_AUTO;
    g_valves[i].type = VALVE_TYPE_PARCEL;

    g_parcels[i].parcel_id = i + 1;
    g_parcels[i].valve_id = i + 1;
    g_parcels[i].enabled = 1;
    snprintf(g_parcels[i].name, 16, "Parcel %d", i + 1);
    g_parcels[i].irrigation_duration_sec = 300;
  }

  /* Dozaj Vanaları Initialize (9-13) - Direct GPIO */
  for (uint8_t i = 0; i < DOSING_VALVE_COUNT; i++) {
    uint8_t valve_id = PARCEL_VALVE_COUNT + i + 1;
    g_valves[valve_id - 1].id = valve_id;
    g_valves[valve_id - 1].state = VALVE_STATE_CLOSED;
    g_valves[valve_id - 1].mode = VALVE_MODE_AUTO;

    if (i == 0) {
      g_valves[valve_id - 1].type = VALVE_TYPE_DOSING_ACID;
    } else {
      g_valves[valve_id - 1].type = VALVE_TYPE_DOSING_FERT;
    }
  }
  PARCELS_Init();
  (void)EEPROM_LoadAllParcels();

  /* Initialize software-driven dosing GPIO pins. PB6/PB7 are TIM4 PWM pins. */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* PC0, PC1, PC2 - spare outputs dedicated to remaining dosing valves */
  GPIO_InitStruct.Pin = DOSING_FERT_B_PIN | DOSING_FERT_C_PIN | DOSING_FERT_D_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* Tüm vanaları kapalı durumda başlat */
  VALVES_CloseAll();

  SYSTEM_LogInfo("VALVES initialized: PCF8574 (parcel) + GPIO (dosing)");
}

static uint8_t VALVES_IsValidValveId(uint8_t valve_id) {
  return (valve_id >= 1U && valve_id <= TOTAL_VALVE_COUNT) ? 1U : 0U;
}

static void VALVES_RecordFault(uint8_t valve_id, uint8_t fault_flag) {
  if (fault_flag == VALVE_FAULT_NONE) {
    return;
  }

  g_error_mask |= fault_flag;

  if (VALVES_IsValidValveId(valve_id) == 0U) {
    return;
  }

  g_valves[valve_id - 1U].fault_flags |= fault_flag;
  g_valves[valve_id - 1U].state = VALVE_STATE_ERROR;
}

static void VALVES_ClearFault(uint8_t valve_id, uint8_t fault_flag) {
  if (VALVES_IsValidValveId(valve_id) == 0U || fault_flag == VALVE_FAULT_NONE) {
    return;
  }

  g_valves[valve_id - 1U].fault_flags &= (uint8_t)(~fault_flag);
  if (g_valves[valve_id - 1U].fault_flags == VALVE_FAULT_NONE &&
      g_valves[valve_id - 1U].state == VALVE_STATE_ERROR) {
    g_valves[valve_id - 1U].state =
        (g_expected_state[valve_id - 1U] != 0U) ? VALVE_STATE_OPEN
                                                : VALVE_STATE_CLOSED;
  }
}

static void VALVES_ClearAllValveFaults(uint8_t fault_flag) {
  for (uint8_t i = 0U; i < TOTAL_VALVE_COUNT; i++) {
    g_valves[i].fault_flags &= (uint8_t)(~fault_flag);
    if (g_valves[i].fault_flags == VALVE_FAULT_NONE &&
        g_valves[i].state == VALVE_STATE_ERROR) {
      g_valves[i].state = (g_expected_state[i] != 0U) ? VALVE_STATE_OPEN
                                                      : VALVE_STATE_CLOSED;
    }
  }
}

/**
 * @brief  Get PCF8574 chip ID for valve (0=parcel, 0xFF=dosing)
 */
static uint8_t VALVES_GetPCFChip(uint8_t valve_id) {
  if (valve_id >= 1U && valve_id <= PARCEL_VALVE_COUNT) {
    return VALVES_PCF_CHIP_PARCEL;
  }
  return 0xFFU;  /* Dosing valves don't use PCF8574 */
}

/**
 * @brief  Get PCF8574 pin number for valve
 */
static uint8_t VALVES_GetPCFPin(uint8_t valve_id) {
  if (valve_id >= 1U && valve_id <= PARCEL_VALVE_COUNT) {
    return (valve_id - 1) & 0x07U;  /* P0-P7 */
  }
  return 0xFFU;  /* Dosing valves don't use PCF8574 */
}

static int8_t VALVES_GetDosingIndex(uint8_t valve_id) {
  if (valve_id < DOSING_VALVE_ACID_ID || valve_id > DOSING_VALVE_FERT_D_ID) {
    return -1;
  }

  return (int8_t)(valve_id - DOSING_VALVE_ACID_ID);
}

static void VALVES_WriteDosingOutput(uint8_t dosing_index, GPIO_PinState state) {
  if (dosing_index >= DOSING_VALVE_COUNT) {
    return;
  }

  if (g_dosing_pwm[dosing_index].hardware_pwm != 0U) {
    g_dosing_pwm[dosing_index].output_state =
        (g_dosing_pwm[dosing_index].applied_duty_percent != 0U) ? 1U : 0U;
    return;
  }

  HAL_GPIO_WritePin(g_dosing_pwm[dosing_index].port, g_dosing_pwm[dosing_index].pin,
                    state);
  g_dosing_pwm[dosing_index].output_state =
      (state == GPIO_PIN_SET) ? 1U : 0U;
}

static void VALVES_ApplyHardwareDosingPWM(uint8_t dosing_index) {
  dosing_pwm_channel_t *channel;
  uint32_t period;
  uint32_t compare;

  if (dosing_index >= DOSING_VALVE_COUNT) {
    return;
  }

  channel = &g_dosing_pwm[dosing_index];
  if (channel->hardware_pwm == 0U || channel->pwm_timer == NULL) {
    return;
  }

  period = __HAL_TIM_GET_AUTORELOAD(channel->pwm_timer) + 1U;
  compare = (period * (uint32_t)channel->applied_duty_percent) / 100U;
  if (compare > period) {
    compare = period;
  }

  __HAL_TIM_SET_COMPARE(channel->pwm_timer, channel->pwm_channel, compare);
  channel->output_state = (compare != 0U) ? 1U : 0U;
}

static uint8_t VALVES_ClampDosingDuty(const dosing_pwm_channel_t *channel,
                                      uint8_t duty_percent) {
  if (channel == NULL || duty_percent == 0U) {
    return 0U;
  }

  if (duty_percent > channel->max_duty_percent) {
    duty_percent = channel->max_duty_percent;
  }

  if (duty_percent < channel->min_duty_percent) {
    duty_percent = channel->min_duty_percent;
  }

  return duty_percent;
}

static void VALVES_RefreshDosingTiming(uint8_t dosing_index) {
  dosing_pwm_channel_t *channel;
  uint32_t period_ticks;
  uint32_t on_ticks;

  if (dosing_index >= DOSING_VALVE_COUNT) {
    return;
  }

  channel = &g_dosing_pwm[dosing_index];

  if (channel->frequency_hz < DOSING_PWM_MIN_FREQ_HZ) {
    channel->frequency_hz = DOSING_PWM_MIN_FREQ_HZ;
  } else if (channel->frequency_hz > DOSING_PWM_MAX_FREQ_HZ) {
    channel->frequency_hz = DOSING_PWM_MAX_FREQ_HZ;
  }

  period_ticks = (SYSTEM_TICK_FREQ_HZ + (channel->frequency_hz / 2U)) /
                 channel->frequency_hz;
  if (period_ticks == 0U) {
    period_ticks = 1U;
  }

  on_ticks = (period_ticks * (uint32_t)channel->applied_duty_percent) / 100U;
  if (channel->applied_duty_percent != 0U && on_ticks == 0U) {
    on_ticks = 1U;
  }

  channel->period_ticks = (uint16_t)period_ticks;
  channel->on_ticks = (uint16_t)on_ticks;
}

static void VALVES_StepDosingRamp(uint8_t dosing_index) {
  dosing_pwm_channel_t *channel;
  uint8_t target_duty;
  uint8_t step;

  if (dosing_index >= DOSING_VALVE_COUNT) {
    return;
  }

  channel = &g_dosing_pwm[dosing_index];
  target_duty = (channel->enabled != 0U)
                    ? VALVES_ClampDosingDuty(channel, channel->target_duty_percent)
                    : 0U;
  step = (channel->ramp_step_percent == 0U) ? DOSING_PWM_DEFAULT_RAMP_STEP
                                            : channel->ramp_step_percent;

  if (channel->applied_duty_percent < target_duty) {
    uint16_t next_duty = (uint16_t)channel->applied_duty_percent + step;
    channel->applied_duty_percent =
        (next_duty > target_duty) ? target_duty : (uint8_t)next_duty;
  } else if (channel->applied_duty_percent > target_duty) {
    if (channel->applied_duty_percent > step) {
      channel->applied_duty_percent -= step;
    } else {
      channel->applied_duty_percent = 0U;
    }
    if (channel->applied_duty_percent < target_duty) {
      channel->applied_duty_percent = target_duty;
    }
  }

  VALVES_RefreshDosingTiming(dosing_index);
  VALVES_ApplyHardwareDosingPWM(dosing_index);
}

static void VALVES_CloseOtherParcelValves(uint8_t keep_open_valve_id) {
  for (uint8_t valve_id = 1U; valve_id <= PARCEL_VALVE_COUNT; valve_id++) {
    if (valve_id == keep_open_valve_id) {
      continue;
    }

    if (g_expected_state[valve_id - 1U] == 0U &&
        g_valves[valve_id - 1U].state != VALVE_STATE_OPEN) {
      continue;
    }

    VALVES_Close(valve_id);
    g_valves[valve_id - 1U].interlock_source_id = keep_open_valve_id;
    VALVES_RecordFault(valve_id, VALVE_FAULT_PARCEL_INTERLOCK);
  }
}

static void VALVES_ApplyDosingInterlock(uint8_t valve_id) {
  if (VALVES_IsDosingValve(valve_id) == 0U) {
    return;
  }

  if (valve_id == DOSING_VALVE_ACID_ID) {
    for (uint8_t fert_id = DOSING_VALVE_FERT_A_ID; fert_id <= DOSING_VALVE_FERT_D_ID;
         fert_id++) {
      if (g_expected_state[fert_id - 1U] == 0U &&
          g_valves[fert_id - 1U].state != VALVE_STATE_OPEN) {
        continue;
      }

      VALVES_Close(fert_id);
      g_valves[fert_id - 1U].interlock_source_id = valve_id;
      VALVES_RecordFault(fert_id, VALVE_FAULT_DOSING_INTERLOCK);
    }
    return;
  }

  if (g_expected_state[DOSING_VALVE_ACID_ID - 1U] != 0U ||
      g_valves[DOSING_VALVE_ACID_ID - 1U].state == VALVE_STATE_OPEN) {
    VALVES_Close(DOSING_VALVE_ACID_ID);
    g_valves[DOSING_VALVE_ACID_ID - 1U].interlock_source_id = valve_id;
    VALVES_RecordFault(DOSING_VALVE_ACID_ID, VALVE_FAULT_DOSING_INTERLOCK);
  }
}

static uint8_t VALVES_ShouldEmulateParcelOutputs(void) {
#if BOARD_IRRIGATION_DEMO_MODE || BOARD_SENSOR_DEMO_MODE
  return 1U;
#else
  return 0U;
#endif
}

static void VALVES_MarkParcelValveOpened(uint8_t valve_id) {
  uint32_t now = HAL_GetTick();

  g_expected_state[valve_id - 1U] = 1U;
  g_last_command_time = now;
  g_valves[valve_id - 1U].cycle_count++;
  g_valves[valve_id - 1U].last_action_time = now;
  g_valves[valve_id - 1U].interlock_source_id = 0U;
  VALVES_ClearFault(valve_id, VALVE_FAULT_DRIVER | VALVE_FAULT_DISABLED |
                                  VALVE_FAULT_STATE_MISMATCH |
                                  VALVE_FAULT_TIMEOUT);
  VALVES_UpdateState(valve_id, VALVE_STATE_OPEN);
  g_consecutive_faults = 0U;
}

static void VALVES_MarkParcelValveClosed(uint8_t valve_id, uint32_t now) {
  g_expected_state[valve_id - 1U] = 0U;
  g_last_command_time = now;
  VALVES_ClearFault(valve_id, VALVE_FAULT_DRIVER | VALVE_FAULT_DISABLED |
                                  VALVE_FAULT_STATE_MISMATCH |
                                  VALVE_FAULT_TIMEOUT);
  VALVES_UpdateState(valve_id, VALVE_STATE_CLOSED);

  if (g_valves[valve_id - 1U].last_action_time > 0U) {
    uint32_t duration = (now - g_valves[valve_id - 1U].last_action_time) / 1000U;
    g_valves[valve_id - 1U].total_on_time += duration;
  }

  g_consecutive_faults = 0U;
}

/**
 * @brief  Open a valve with interlock and fault handling.
 */
uint8_t VALVES_RequestOpen(uint8_t valve_id) {
  uint8_t chip_id;
  uint8_t pin;

  if (VALVES_IsValidValveId(valve_id) == 0U) {
    SYSTEM_LogError("Invalid valve ID: %u", valve_id);
    g_error_mask |= VALVE_FAULT_INVALID_ID;
    return 0U;
  }

  if (g_valves[valve_id - 1U].mode == VALVE_MODE_DISABLED) {
    VALVES_RecordFault(valve_id, VALVE_FAULT_DISABLED);
    SYSTEM_LogWarn("Valve %u is disabled", valve_id);
    return 0U;
  }

  chip_id = VALVES_GetPCFChip(valve_id);
  pin = VALVES_GetPCFPin(valve_id);

  if (VALVES_IsDosingValve(valve_id) == 0U) {
    VALVES_CloseOtherParcelValves(valve_id);
    VALVES_ClearFault(valve_id, VALVE_FAULT_PARCEL_INTERLOCK);
  } else {
    VALVES_ApplyDosingInterlock(valve_id);
    VALVES_ClearFault(valve_id, VALVE_FAULT_DOSING_INTERLOCK);
  }

  /* Dosing valves are driven directly from GPIO/PWM. */
  if (chip_id == 0xFFU) {
    int8_t dosing_index = VALVES_GetDosingIndex(valve_id);
    dosing_pwm_channel_t *channel;
    uint32_t now = HAL_GetTick();

    if (dosing_index < 0) {
      VALVES_RecordFault(valve_id, VALVE_FAULT_INVALID_ID);
      return 0U;
    }

    channel = &g_dosing_pwm[dosing_index];
    if (channel->target_duty_percent == 0U) {
      channel->target_duty_percent = DOSING_PWM_DEFAULT_DUTY;
    }

    if (channel->enabled == 0U) {
      g_valves[valve_id - 1U].cycle_count++;
      g_valves[valve_id - 1U].last_action_time = now;
      channel->tick_in_period = 0U;
    }

    channel->enabled = 1U;
    VALVES_StepDosingRamp((uint8_t)dosing_index);
    g_expected_state[valve_id - 1U] = 1U;
    g_last_command_time = now;
    g_valves[valve_id - 1U].interlock_source_id = 0U;
    VALVES_ClearFault(valve_id, VALVE_FAULT_DRIVER | VALVE_FAULT_DISABLED |
                                    VALVE_FAULT_STATE_MISMATCH |
                                    VALVE_FAULT_TIMEOUT);
    VALVES_UpdateState(valve_id, VALVE_STATE_OPEN);
    g_consecutive_faults = 0U;

    SYSTEM_LogInfo("Dosing valve %u OPENED (%u%% @ %uHz)", valve_id,
                   channel->target_duty_percent, channel->frequency_hz);
    return 1U;
  }

  /* Parcel valves are driven through PCF8574. */
  pcf8574_handle_t *hpcf = PCF8574_GetChip(chip_id);
  if (hpcf == NULL || !hpcf->is_initialized) {
    if (VALVES_ShouldEmulateParcelOutputs() != 0U) {
      VALVES_MarkParcelValveOpened(valve_id);
      SYSTEM_LogWarn("Valve %u OPEN emulated: PCF8574 #%u not available",
                     valve_id, chip_id);
      return 1U;
    }

    SYSTEM_LogError("PCF8574 #%u not available", chip_id);
    VALVES_RecordFault(valve_id, VALVE_FAULT_DRIVER);
    g_consecutive_faults++;
    return 0U;
  }

  if (PCF8574_WritePin(hpcf, pin, 1U) != PCF8574_OK) {
    if (VALVES_ShouldEmulateParcelOutputs() != 0U) {
      VALVES_MarkParcelValveOpened(valve_id);
      SYSTEM_LogWarn("Valve %u OPEN emulated: PCF8574 write failed",
                     valve_id);
      return 1U;
    }

    SYSTEM_LogError("Failed to open valve %u", valve_id);
    VALVES_RecordFault(valve_id, VALVE_FAULT_DRIVER);
    g_consecutive_faults++;
    return 0U;
  }

  VALVES_MarkParcelValveOpened(valve_id);

  SYSTEM_LogInfo("Valve %u OPENED (PCF #%u, P%u)", valve_id, chip_id, pin);
  return 1U;
}

/**
 * @brief  Close a valve and clear runtime output state.
 */
uint8_t VALVES_RequestClose(uint8_t valve_id) {
  uint8_t chip_id;
  uint8_t pin;
  uint32_t now;

  if (VALVES_IsValidValveId(valve_id) == 0U) {
    g_error_mask |= VALVE_FAULT_INVALID_ID;
    return 0U;
  }

  chip_id = VALVES_GetPCFChip(valve_id);
  pin = VALVES_GetPCFPin(valve_id);
  now = HAL_GetTick();

  if (chip_id == 0xFFU) {
    int8_t dosing_index = VALVES_GetDosingIndex(valve_id);
    dosing_pwm_channel_t *channel;

    if (dosing_index < 0) {
      VALVES_RecordFault(valve_id, VALVE_FAULT_INVALID_ID);
      return 0U;
    }

    channel = &g_dosing_pwm[dosing_index];
    channel->enabled = 0U;
    channel->applied_duty_percent = 0U;
    channel->tick_in_period = 0U;
    VALVES_RefreshDosingTiming((uint8_t)dosing_index);
    VALVES_ApplyHardwareDosingPWM((uint8_t)dosing_index);
    VALVES_WriteDosingOutput((uint8_t)dosing_index, GPIO_PIN_RESET);
    g_expected_state[valve_id - 1U] = 0U;
    g_last_command_time = now;
    VALVES_ClearFault(valve_id, VALVE_FAULT_DRIVER | VALVE_FAULT_DISABLED |
                                    VALVE_FAULT_STATE_MISMATCH |
                                    VALVE_FAULT_TIMEOUT);
    VALVES_UpdateState(valve_id, VALVE_STATE_CLOSED);

    if (g_valves[valve_id - 1U].last_action_time > 0U) {
      uint32_t duration = (now - g_valves[valve_id - 1U].last_action_time) / 1000U;
      g_valves[valve_id - 1U].total_on_time += duration;
    }

    g_consecutive_faults = 0U;
    SYSTEM_LogInfo("Dosing valve %u CLOSED (GPIO)", valve_id);
    return 1U;
  }

  pcf8574_handle_t *hpcf = PCF8574_GetChip(chip_id);
  if (hpcf == NULL || !hpcf->is_initialized) {
    if (VALVES_ShouldEmulateParcelOutputs() != 0U) {
      VALVES_MarkParcelValveClosed(valve_id, now);
      SYSTEM_LogWarn("Valve %u CLOSE emulated: PCF8574 #%u not available",
                     valve_id, chip_id);
      return 1U;
    }

    VALVES_RecordFault(valve_id, VALVE_FAULT_DRIVER);
    g_consecutive_faults++;
    return 0U;
  }

  if (PCF8574_WritePin(hpcf, pin, 0U) != PCF8574_OK) {
    if (VALVES_ShouldEmulateParcelOutputs() != 0U) {
      VALVES_MarkParcelValveClosed(valve_id, now);
      SYSTEM_LogWarn("Valve %u CLOSE emulated: PCF8574 write failed",
                     valve_id);
      return 1U;
    }

    SYSTEM_LogError("Failed to close valve %u", valve_id);
    VALVES_RecordFault(valve_id, VALVE_FAULT_DRIVER);
    g_consecutive_faults++;
    return 0U;
  }

  VALVES_MarkParcelValveClosed(valve_id, now);
  SYSTEM_LogInfo("Valve %u CLOSED (PCF #%u, P%u)", valve_id, chip_id, pin);
  return 1U;
}

void VALVES_Open(uint8_t valve_id) { (void)VALVES_RequestOpen(valve_id); }

void VALVES_Close(uint8_t valve_id) { (void)VALVES_RequestClose(valve_id); }

/**
 * @brief  Close all valves (emergency stop)
 */
void VALVES_CloseAll(void) {
  /* Close PCF8574 parcel valves */
  for (uint8_t chip_id = 0; chip_id < PCF8574_GetChipCount(); chip_id++) {
    pcf8574_handle_t *hpcf = PCF8574_GetChip(chip_id);

    if (hpcf != NULL && hpcf->is_initialized) {
      PCF8574_WriteByte(hpcf, 0x00U);  /* All outputs LOW */
    }
  }

  /* Close dosing valves (direct GPIO) */
  for (uint8_t i = 0; i < DOSING_VALVE_COUNT; i++) {
    g_dosing_pwm[i].enabled = 0U;
    g_dosing_pwm[i].applied_duty_percent = 0U;
    g_dosing_pwm[i].tick_in_period = 0U;
    VALVES_RefreshDosingTiming(i);
    VALVES_ApplyHardwareDosingPWM(i);
    VALVES_WriteDosingOutput(i, GPIO_PIN_RESET);
  }

  /* Update expected state */
  memset(g_expected_state, 0, sizeof(g_expected_state));
  g_last_command_time = HAL_GetTick();

  /* Update internal state for all valves */
  for (uint8_t i = 0; i < TOTAL_VALVE_COUNT; i++) {
    if (g_valves[i].id != 0U) {
      g_valves[i].interlock_source_id = 0U;
      VALVES_UpdateState(i + 1, VALVE_STATE_CLOSED);
    }
  }

  SYSTEM_LogInfo("ALL VALVES CLOSED (Emergency Stop)");
}

void VALVES_StopAll(void) { VALVES_CloseAll(); }

/**
 * @brief  Get current state of a valve
 */
uint8_t VALVES_GetState(uint8_t valve_id) {
  if (valve_id == 0U || valve_id > TOTAL_VALVE_COUNT) {
    return VALVE_STATE_ERROR;
  }
  return g_valves[valve_id - 1].state;
}

uint8_t VALVES_GetFaultFlags(uint8_t valve_id) {
  if (VALVES_IsValidValveId(valve_id) == 0U) {
    return VALVE_FAULT_INVALID_ID;
  }

  return g_valves[valve_id - 1U].fault_flags;
}

void VALVES_ClearFaultFlags(uint8_t valve_id) {
  if (VALVES_IsValidValveId(valve_id) == 0U) {
    return;
  }

  g_valves[valve_id - 1U].fault_flags = VALVE_FAULT_NONE;
  g_valves[valve_id - 1U].interlock_source_id = 0U;
  if (g_valves[valve_id - 1U].state == VALVE_STATE_ERROR) {
    g_valves[valve_id - 1U].state =
        (g_expected_state[valve_id - 1U] != 0U) ? VALVE_STATE_OPEN
                                                : VALVE_STATE_CLOSED;
  }
}

uint8_t VALVES_GetInterlockSource(uint8_t valve_id) {
  if (VALVES_IsValidValveId(valve_id) == 0U) {
    return 0U;
  }

  return g_valves[valve_id - 1U].interlock_source_id;
}

void VALVES_Update(void) {
  /* Timer-driven PWM is serviced from TIM3 at 1 ms resolution. */
}

void VALVES_TimerTick1ms(void) {
  for (uint8_t i = 0; i < DOSING_VALVE_COUNT; i++) {
    dosing_pwm_channel_t *channel = &g_dosing_pwm[i];

    if (channel->enabled == 0U) {
      channel->applied_duty_percent = 0U;
      channel->tick_in_period = 0U;
      VALVES_ApplyHardwareDosingPWM(i);
      VALVES_WriteDosingOutput(i, GPIO_PIN_RESET);
      continue;
    }

    if (channel->tick_in_period == 0U) {
      VALVES_StepDosingRamp(i);
    }

    if (channel->hardware_pwm != 0U) {
      channel->tick_in_period++;
      if (channel->tick_in_period >= channel->period_ticks) {
        channel->tick_in_period = 0U;
      }
      continue;
    }

    if (channel->applied_duty_percent == 0U || channel->on_ticks == 0U) {
      VALVES_WriteDosingOutput(i, GPIO_PIN_RESET);
    } else if (channel->on_ticks >= channel->period_ticks) {
      VALVES_WriteDosingOutput(i, GPIO_PIN_SET);
    } else if (channel->tick_in_period < channel->on_ticks) {
      VALVES_WriteDosingOutput(i, GPIO_PIN_SET);
    } else {
      VALVES_WriteDosingOutput(i, GPIO_PIN_RESET);
    }

    channel->tick_in_period++;
    if (channel->tick_in_period >= channel->period_ticks) {
      channel->tick_in_period = 0U;
    }
  }
}

/**
 * @brief  Internal state update
 */
static void VALVES_UpdateState(uint8_t id, valve_state_t new_state) {
  if (id == 0U || id > TOTAL_VALVE_COUNT) {
    return;
  }
  g_valves[id - 1].state = new_state;

  /* GUI'ye bildir (sadece parsel vanaları için) */
  if (id <= PARCEL_VALVE_COUNT) {
    GUI_UpdateValveStatus(id, (new_state == VALVE_STATE_OPEN) ? 1U : 0U);
  }
}

/**
 * @brief  Calculate CRC of commanded valve states
 */
static uint16_t VALVES_CalculateCommandCRC(void) {
  uint16_t crc = 0xFFFFU;

  for (uint8_t i = 0; i < TOTAL_VALVE_COUNT; i++) {
    crc ^= (uint16_t)g_expected_state[i] << 8U;

    for (uint8_t j = 0; j < 8U; j++) {
      if (crc & 0x8000U) {
        crc = (crc << 1U) ^ 0x1021U;
      } else {
        crc <<= 1U;
      }
    }
  }

  return crc;
}

/**
 * @brief  Safety validation - verify valve states match commands
 */
void VALVES_SafetyCheck(void) {
  static uint32_t last_check = 0U;
  uint32_t now = HAL_GetTick();

  /* Check every 500ms */
  if (now - last_check < 500U) {
    return;
  }
  last_check = now;

  /* 1. Check for command timeout (no commands in 10 seconds while running) */
  if (IRRIGATION_CTRL_IsRunning() &&
      (now - g_last_command_time > 10000U)) {
    SYSTEM_LogWarn("Valve command timeout - refreshing states");
    g_error_mask |= VALVE_FAULT_TIMEOUT;

    /* Refresh all valve states */
    for (uint8_t i = 0; i < TOTAL_VALVE_COUNT; i++) {
      if (g_expected_state[i] != g_valves[i].state) {
        uint8_t valve_id = i + 1;
        VALVES_RecordFault(valve_id, VALVE_FAULT_TIMEOUT);
        if (g_expected_state[i]) {
          VALVES_Open(valve_id);
        } else {
          VALVES_Close(valve_id);
        }
      }
    }
  }

  /* 2. Check for consecutive faults */
  if (g_consecutive_faults >= 5U) {
    SYSTEM_LogError("Too many valve faults - EMERGENCY STOP");
    g_error_mask |= VALVE_FAULT_DRIVER | VALVE_FAULT_EMERGENCY_STOP;
    VALVES_CloseAll();
    IRRIGATION_CTRL_Stop();
    GUI_ShowError("VALVE COMMUNICATION FAULT");
  }

  /* 3. Periodic PCF8574 health check */
  PCF8574_PeriodicTask();

  /* 4. Validate CRC */
  g_valve_command_crc = VALVES_CalculateCommandCRC();
}

/**
 * @brief  Get valve statistics
 */
uint32_t VALVES_GetTotalOnTime(uint8_t valve_id) {
  if (valve_id == 0U || valve_id > TOTAL_VALVE_COUNT) {
    return 0U;
  }
  return g_valves[valve_id - 1].total_on_time;
}

/**
 * @brief  Get valve cycle count
 */
uint32_t VALVES_GetCycleCount(uint8_t valve_id) {
  if (valve_id == 0U || valve_id > TOTAL_VALVE_COUNT) {
    return 0U;
  }
  return g_valves[valve_id - 1].cycle_count;
}

/**
 * @brief  Get active valve mask (for diagnostics)
 */
uint16_t VALVES_GetActiveMask(void) {
  uint16_t mask = 0U;

  for (uint8_t i = 0; i < TOTAL_VALVE_COUNT; i++) {
    if (g_valves[i].state == VALVE_STATE_OPEN) {
      mask |= (1U << i);
    }
  }

  return mask;
}

/**
 * @brief  Get expected valve mask (for validation)
 */
uint16_t VALVES_GetExpectedMask(void) {
  uint16_t mask = 0U;

  for (uint8_t i = 0; i < TOTAL_VALVE_COUNT; i++) {
    if (g_expected_state[i]) {
      mask |= (1U << i);
    }
  }

  return mask;
}

/**
 * @brief  Diagnostic: Check if actual state matches expected state
 */
uint8_t VALVES_ValidateStates(void) {
  uint16_t active_mask = VALVES_GetActiveMask();
  uint16_t expected_mask = VALVES_GetExpectedMask();

  if (active_mask != expected_mask) {
    SYSTEM_LogError("VALVE STATE MISMATCH! Active: 0x%04X, Expected: 0x%04X",
                    active_mask, expected_mask);
    g_error_mask |= VALVE_FAULT_STATE_MISMATCH;
    for (uint8_t i = 0U; i < TOTAL_VALVE_COUNT; i++) {
      uint8_t active = ((active_mask & (1U << i)) != 0U) ? 1U : 0U;
      uint8_t expected = ((expected_mask & (1U << i)) != 0U) ? 1U : 0U;

      if (active != expected) {
        VALVES_RecordFault((uint8_t)(i + 1U), VALVE_FAULT_STATE_MISMATCH);
      }
    }
    return 0U;
  }

  g_error_mask &= (uint8_t)(~VALVE_FAULT_STATE_MISMATCH);
  VALVES_ClearAllValveFaults(VALVE_FAULT_STATE_MISMATCH);

  return 1U;
}

/**
 * @brief  Manual valve control
 */
void VALVES_ManualOpen(uint8_t valve_id) {
  VALVES_Open(valve_id);
}

void VALVES_ManualClose(uint8_t valve_id) {
  VALVES_Close(valve_id);
}

/**
 * @brief  Emergency stop
 */
void VALVES_EmergencyStop(void) {
  g_error_mask |= VALVE_FAULT_EMERGENCY_STOP;
  for (uint8_t i = 0U; i < TOTAL_VALVE_COUNT; i++) {
    VALVES_RecordFault((uint8_t)(i + 1U), VALVE_FAULT_EMERGENCY_STOP);
  }
  VALVES_CloseAll();
}

void VALVES_OpenAll(void) {
  for (uint8_t valve_id = 1U; valve_id <= TOTAL_VALVE_COUNT; valve_id++) {
    VALVES_Open(valve_id);
  }
}

void VALVES_SetActiveMask(uint16_t mask) {
  uint8_t selected_parcel = 0U;
  uint8_t acid_requested = ((mask & (1U << (DOSING_VALVE_ACID_ID - 1U))) != 0U) ? 1U
                                                                                  : 0U;

  VALVES_CloseAll();

  for (uint8_t valve_id = 1U; valve_id <= PARCEL_VALVE_COUNT; valve_id++) {
    if ((mask & (1U << (valve_id - 1U))) == 0U) {
      continue;
    }

    selected_parcel = valve_id;
    break;
  }

  if (selected_parcel != 0U) {
    VALVES_Open(selected_parcel);
  }

  if (acid_requested != 0U) {
    VALVES_Open(DOSING_VALVE_ACID_ID);
    return;
  }

  for (uint8_t valve_id = DOSING_VALVE_FERT_A_ID; valve_id <= DOSING_VALVE_FERT_D_ID;
       valve_id++) {
    if ((mask & (1U << (valve_id - 1U))) != 0U) {
      VALVES_Open(valve_id);
    }
  }
}

void VALVES_SetMode(uint8_t valve_id, valve_mode_t mode) {
  if (VALVES_IsValidValveId(valve_id) == 0U) {
    return;
  }

  g_valves[valve_id - 1U].mode = mode;
  if (mode == VALVE_MODE_DISABLED) {
    VALVES_Close(valve_id);
  }
}

valve_mode_t VALVES_GetMode(uint8_t valve_id) {
  if (VALVES_IsValidValveId(valve_id) == 0U) {
    return VALVE_MODE_DISABLED;
  }

  return g_valves[valve_id - 1U].mode;
}

void VALVES_SetTiming(uint8_t valve_id, uint32_t on_ms, uint32_t off_ms) {
  if (VALVES_IsValidValveId(valve_id) == 0U) {
    return;
  }

  g_valves[valve_id - 1U].on_time_ms = on_ms;
  g_valves[valve_id - 1U].off_time_ms = off_ms;
}

uint8_t VALVES_IsManualMode(uint8_t valve_id) {
  if (VALVES_IsValidValveId(valve_id) == 0U) {
    return 0U;
  }

  return (g_valves[valve_id - 1U].mode == VALVE_MODE_MANUAL) ? 1U : 0U;
}

void VALVES_CheckErrors(void) {
  (void)VALVES_ValidateStates();
  VALVES_SafetyCheck();
}

uint8_t VALVES_GetErrorMask(void) { return g_error_mask; }

void VALVES_ClearErrors(void) {
  g_error_mask = 0U;
  g_consecutive_faults = 0U;
  for (uint8_t i = 0U; i < TOTAL_VALVE_COUNT; i++) {
    g_valves[i].fault_flags = VALVE_FAULT_NONE;
    g_valves[i].interlock_source_id = 0U;
    if (g_valves[i].state == VALVE_STATE_ERROR) {
      g_valves[i].state = (g_expected_state[i] != 0U) ? VALVE_STATE_OPEN
                                                      : VALVE_STATE_CLOSED;
    }
  }
}

void VALVES_ResetStatistics(uint8_t valve_id) {
  if (VALVES_IsValidValveId(valve_id) != 0U) {
    g_valves[valve_id - 1U].total_on_time = 0U;
    g_valves[valve_id - 1U].cycle_count = 0U;
    return;
  }

  for (uint8_t i = 0U; i < TOTAL_VALVE_COUNT; i++) {
    g_valves[i].total_on_time = 0U;
    g_valves[i].cycle_count = 0U;
  }
}

/**
 * @brief  Parcel functions
 */
void PARCELS_Init(void) {
  for (uint8_t i = 0U; i < PARCEL_VALVE_COUNT; i++) {
    g_parcels[i].parcel_id = (uint8_t)(i + 1U);
    g_parcels[i].valve_id = (uint8_t)(i + 1U);
    g_parcels[i].enabled = 1U;
    g_parcels[i].is_watering = 0U;
    g_parcels[i].wait_duration_sec = 0U;
    if (g_parcels[i].irrigation_duration_sec == 0U) {
      g_parcels[i].irrigation_duration_sec = 300U;
    }
    if (g_parcels[i].name[0] == '\0') {
      (void)snprintf(g_parcels[i].name, sizeof(g_parcels[i].name), "Parcel %u",
                     i + 1U);
    }
  }
}

void PARCELS_SetDuration(uint8_t parcel_id, uint32_t duration_sec) {
  if (parcel_id == 0U || parcel_id > PARCEL_VALVE_COUNT) return;
  g_parcels[parcel_id - 1].irrigation_duration_sec = duration_sec;
}

uint32_t PARCELS_GetDuration(uint8_t parcel_id) {
  if (parcel_id == 0U || parcel_id > PARCEL_VALVE_COUNT) return 0U;
  return g_parcels[parcel_id - 1].irrigation_duration_sec;
}

void PARCELS_SetEnabled(uint8_t parcel_id, uint8_t enabled) {
  if (parcel_id == 0U || parcel_id > PARCEL_VALVE_COUNT) return;
  g_parcels[parcel_id - 1].enabled = (enabled != 0U) ? 1U : 0U;
}

uint8_t PARCELS_IsEnabled(uint8_t parcel_id) {
  if (parcel_id == 0U || parcel_id > PARCEL_VALVE_COUNT) return 0U;
  return g_parcels[parcel_id - 1].enabled;
}

void PARCELS_SetName(uint8_t parcel_id, const char *name) {
  if (parcel_id == 0U || parcel_id > PARCEL_VALVE_COUNT || name == NULL) {
    return;
  }

  (void)snprintf(g_parcels[parcel_id - 1U].name,
                 sizeof(g_parcels[parcel_id - 1U].name), "%s", name);
}

const char *PARCELS_GetName(uint8_t parcel_id) {
  if (parcel_id == 0U || parcel_id > PARCEL_VALVE_COUNT) {
    return "";
  }

  return g_parcels[parcel_id - 1U].name;
}

uint8_t VALVES_IsDosingValve(uint8_t valve_id) {
  return (VALVES_GetDosingIndex(valve_id) >= 0) ? 1U : 0U;
}

void VALVES_OpenDosing(uint8_t dosing_idx) {
  if (dosing_idx >= DOSING_VALVE_COUNT) {
    return;
  }

  VALVES_Open((uint8_t)(DOSING_VALVE_ACID_ID + dosing_idx));
}

void VALVES_CloseDosing(uint8_t dosing_idx) {
  if (dosing_idx >= DOSING_VALVE_COUNT) {
    return;
  }

  VALVES_Close((uint8_t)(DOSING_VALVE_ACID_ID + dosing_idx));
}

void VALVES_ConfigureDosingChannel(uint8_t valve_id, uint8_t frequency_hz,
                                   uint8_t min_duty_percent,
                                   uint8_t max_duty_percent,
                                   uint8_t ramp_step_percent) {
  int8_t dosing_index = VALVES_GetDosingIndex(valve_id);
  dosing_pwm_channel_t *channel;

  if (dosing_index < 0) {
    return;
  }

  channel = &g_dosing_pwm[dosing_index];
  if (channel->hardware_pwm != 0U) {
    frequency_hz = DOSING_HW_PWM_FREQ_HZ;
  }

  if (frequency_hz < DOSING_PWM_MIN_FREQ_HZ) {
    frequency_hz = DOSING_PWM_MIN_FREQ_HZ;
  } else if (frequency_hz > DOSING_PWM_MAX_FREQ_HZ) {
    frequency_hz = DOSING_PWM_MAX_FREQ_HZ;
  }

  if (min_duty_percent > 100U) {
    min_duty_percent = 100U;
  }
  if (max_duty_percent == 0U || max_duty_percent > 100U) {
    max_duty_percent = 100U;
  }
  if (min_duty_percent > max_duty_percent) {
    min_duty_percent = max_duty_percent;
  }
  if (ramp_step_percent == 0U) {
    ramp_step_percent = DOSING_PWM_DEFAULT_RAMP_STEP;
  }

  channel->frequency_hz = frequency_hz;
  channel->min_duty_percent = min_duty_percent;
  channel->max_duty_percent = max_duty_percent;
  channel->ramp_step_percent = ramp_step_percent;
  channel->target_duty_percent =
      VALVES_ClampDosingDuty(channel, channel->target_duty_percent);
  channel->applied_duty_percent =
      VALVES_ClampDosingDuty(channel, channel->applied_duty_percent);
  VALVES_RefreshDosingTiming((uint8_t)dosing_index);
  VALVES_ApplyHardwareDosingPWM((uint8_t)dosing_index);
}

void VALVES_SetDosingDuty(uint8_t valve_id, uint8_t duty_percent) {
  int8_t dosing_index = VALVES_GetDosingIndex(valve_id);
  dosing_pwm_channel_t *channel;

  if (dosing_index < 0) {
    return;
  }

  channel = &g_dosing_pwm[dosing_index];
  channel->target_duty_percent = VALVES_ClampDosingDuty(channel, duty_percent);
  if (duty_percent == 0U) {
    channel->target_duty_percent = 0U;
  }
  VALVES_RefreshDosingTiming((uint8_t)dosing_index);
}

uint8_t VALVES_GetDosingDuty(uint8_t valve_id) {
  int8_t dosing_index = VALVES_GetDosingIndex(valve_id);

  if (dosing_index < 0) {
    return 0U;
  }

  return g_dosing_pwm[dosing_index].target_duty_percent;
}

uint8_t VALVES_GetDosingAppliedDuty(uint8_t valve_id) {
  int8_t dosing_index = VALVES_GetDosingIndex(valve_id);

  if (dosing_index < 0) {
    return 0U;
  }

  return g_dosing_pwm[dosing_index].applied_duty_percent;
}

void VALVES_SetDosingFrequency(uint8_t valve_id, uint8_t frequency_hz) {
  int8_t dosing_index = VALVES_GetDosingIndex(valve_id);

  if (dosing_index < 0) {
    return;
  }

  VALVES_ConfigureDosingChannel(
      valve_id, frequency_hz, g_dosing_pwm[dosing_index].min_duty_percent,
      g_dosing_pwm[dosing_index].max_duty_percent,
      g_dosing_pwm[dosing_index].ramp_step_percent);
}

uint8_t VALVES_GetDosingFrequency(uint8_t valve_id) {
  int8_t dosing_index = VALVES_GetDosingIndex(valve_id);

  if (dosing_index < 0) {
    return 0U;
  }

  return g_dosing_pwm[dosing_index].frequency_hz;
}

uint8_t VALVES_IsDosingEnabled(uint8_t valve_id) {
  int8_t dosing_index = VALVES_GetDosingIndex(valve_id);

  if (dosing_index < 0) {
    return 0U;
  }

  return g_dosing_pwm[dosing_index].enabled;
}

uint8_t VALVES_GetDosingOutputState(uint8_t valve_id) {
  int8_t dosing_index = VALVES_GetDosingIndex(valve_id);

  if (dosing_index < 0) {
    return 0U;
  }

  return g_dosing_pwm[dosing_index].output_state;
}

void VALVES_GetDosingStatus(uint8_t valve_id, dosing_channel_status_t *status) {
  int8_t dosing_index = VALVES_GetDosingIndex(valve_id);

  if (status == NULL) {
    return;
  }

  memset(status, 0, sizeof(*status));
  if (dosing_index < 0) {
    return;
  }

  status->frequency_hz = g_dosing_pwm[dosing_index].frequency_hz;
  status->min_duty_percent = g_dosing_pwm[dosing_index].min_duty_percent;
  status->max_duty_percent = g_dosing_pwm[dosing_index].max_duty_percent;
  status->target_duty_percent = g_dosing_pwm[dosing_index].target_duty_percent;
  status->applied_duty_percent = g_dosing_pwm[dosing_index].applied_duty_percent;
  status->ramp_step_percent = g_dosing_pwm[dosing_index].ramp_step_percent;
  status->enabled = g_dosing_pwm[dosing_index].enabled;
  status->output_state = g_dosing_pwm[dosing_index].output_state;
}
