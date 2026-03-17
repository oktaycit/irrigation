/**
 ******************************************************************************
 * @file           : irrigation_control.c
 * @brief          : Sulama Algoritması ve Kontrol Mantığı Implementation
 ******************************************************************************
 */

#include "main.h"
#include "irrigation_control.h"
#include <string.h>
#include <math.h>

/* Internal Control Structure */
static irrigation_control_t ctrl = {0};
static auto_mode_t g_auto_mode = AUTO_MODE_OFF;
static uint8_t g_active_dosing_valve = 0U;
static irrigation_schedule_t g_schedules[VALVE_COUNT] = {0};

static void IRRIGATION_CTRL_ChangeState(control_state_t new_state);
static void IRRIGATION_CTRL_CompleteDoseIfNeeded(void);
static void IRRIGATION_CTRL_UpdateDoseState(uint8_t valve_id, uint32_t dose_time_ms);
static void IRRIGATION_CTRL_ResetCurrentParcel(void);
static uint8_t IRRIGATION_CTRL_IsParcelQueued(uint8_t parcel_id);

/**
 * @brief  Initialize Irrigation Controller
 */
void IRRIGATION_CTRL_Init(void) {
    memset(&ctrl, 0, sizeof(irrigation_control_t));
    ctrl.state = CTRL_STATE_IDLE;
    ctrl.ph_params.target = 6.5f;
    ctrl.ph_params.min_limit = 5.5f;
    ctrl.ph_params.max_limit = 7.5f;
    ctrl.ph_params.hysteresis = IRRIGATION_PH_HYSTERESIS;
    ctrl.ph_params.dose_time_ms = 500U;
    ctrl.ec_params.target = 1.8f;
    ctrl.ec_params.min_limit = 1.0f;
    ctrl.ec_params.max_limit = 2.5f;
    ctrl.ec_params.hysteresis = IRRIGATION_EC_HYSTERESIS;
    ctrl.ec_params.dose_time_ms = 500U;
    ctrl.ph_params.wait_time_ms = IRRIGATION_MIXING_TIME * 1000U;
    ctrl.ec_params.wait_time_ms = IRRIGATION_MIXING_TIME * 1000U;
    ctrl.ph_params.auto_adjust_enabled = 1U;
    ctrl.ec_params.auto_adjust_enabled = 1U;
    g_active_dosing_valve = 0U;
    
    /* Load parameters from EEPROM */
    EEPROM_LoadSystemParams();
}

void IRRIGATION_CTRL_SetPHParams(float target, float min, float max,
                                 float hyst) {
    if (min > max) {
        float tmp = min;
        min = max;
        max = tmp;
    }

    if (target < min) target = min;
    if (target > max) target = max;

    ctrl.ph_params.target = target;
    ctrl.ph_params.min_limit = min;
    ctrl.ph_params.max_limit = max;
    if (hyst > 0.0f) {
        ctrl.ph_params.hysteresis = hyst;
    }
}

void IRRIGATION_CTRL_SetECParams(float target, float min, float max,
                                 float hyst) {
    if (min > max) {
        float tmp = min;
        min = max;
        max = tmp;
    }

    if (target < min) target = min;
    if (target > max) target = max;

    ctrl.ec_params.target = target;
    ctrl.ec_params.min_limit = min;
    ctrl.ec_params.max_limit = max;
    if (hyst > 0.0f) {
        ctrl.ec_params.hysteresis = hyst;
    }
}

void IRRIGATION_CTRL_GetPHParams(ph_control_params_t *params) {
    if (params == NULL) return;
    *params = ctrl.ph_params;
}

void IRRIGATION_CTRL_GetECParams(ec_control_params_t *params) {
    if (params == NULL) return;
    *params = ctrl.ec_params;
}

void IRRIGATION_CTRL_SetParcelDuration(uint8_t parcel_id,
                                       uint32_t duration_sec) {
    if (duration_sec == 0U) {
        duration_sec = 30U;
    }

    PARCELS_SetDuration(parcel_id, duration_sec);
}

/**
 * @brief  Main Control Update (Called periodically)
 */
void IRRIGATION_CTRL_Update(void) {
    if (!ctrl.is_running || ctrl.is_paused) return;

    switch (ctrl.state) {
        case CTRL_STATE_IDLE:
            /* Transition to sensor check if it's time */
            IRRIGATION_CTRL_ChangeState(CTRL_STATE_CHECKING_SENSORS);
            break;

        case CTRL_STATE_CHECKING_SENSORS:
            IRRIGATION_CTRL_ReadSensors();
            if (!IRRIGATION_CTRL_IsPHValid()) {
                IRRIGATION_CTRL_SetError(ERR_PH_SENSOR_FAULT);
            } else if (!IRRIGATION_CTRL_IsECValid()) {
                IRRIGATION_CTRL_SetError(ERR_EC_SENSOR_FAULT);
            } else if (IRRIGATION_CTRL_IsPHInRange() && IRRIGATION_CTRL_IsECInRange()) {
                IRRIGATION_CTRL_ChangeState(CTRL_STATE_PARCEL_WATERING);
            } else if (!IRRIGATION_CTRL_IsPHInRange()) {
                IRRIGATION_CTRL_ChangeState(CTRL_STATE_PH_ADJUSTING);
            } else {
                IRRIGATION_CTRL_ChangeState(CTRL_STATE_EC_ADJUSTING);
            }
            break;

        case CTRL_STATE_PH_ADJUSTING:
            IRRIGATION_CTRL_AdjustPH();
            break;

        case CTRL_STATE_EC_ADJUSTING:
            IRRIGATION_CTRL_AdjustEC();
            break;

        case CTRL_STATE_MIXING:
            if (IRRIGATION_CTRL_IsMixingComplete() != 0U) {
                IRRIGATION_CTRL_StartSettling();
            }
            break;

        case CTRL_STATE_SETTLING:
            if (IRRIGATION_CTRL_IsSettlingComplete() != 0U) {
                IRRIGATION_CTRL_ChangeState(CTRL_STATE_CHECKING_SENSORS);
            }
            break;

        case CTRL_STATE_PARCEL_WATERING:
            if (ctrl.current_parcel.parcel_id == 0) {
                /* Start first available parcel from queue or sequence */
                IRRIGATION_CTRL_NextParcel();
                if (ctrl.current_parcel.parcel_id == 0U) {
                    ctrl.is_running = 0U;
                    IRRIGATION_CTRL_ChangeState(CTRL_STATE_IDLE);
                    break;
                }
            }
            
            if (IRRIGATION_CTRL_IsParcelComplete()) {
                VALVES_Close(ctrl.current_parcel.valve_id);
                ctrl.total_parcel_count++;
                IRRIGATION_CTRL_OnParcelComplete(ctrl.current_parcel.parcel_id);
                if (ctrl.queue_size > 0) {
                    IRRIGATION_CTRL_ResetCurrentParcel();
                    IRRIGATION_CTRL_NextParcel();
                } else {
                    IRRIGATION_CTRL_ResetCurrentParcel();
                    IRRIGATION_CTRL_ChangeState(CTRL_STATE_IDLE);
                    ctrl.is_running = 0;
                }
            }
            break;

        case CTRL_STATE_ERROR:
        case CTRL_STATE_EMERGENCY_STOP:
            VALVES_EmergencyStop();
            ctrl.is_running = 0U;
            break;

        default:
            IRRIGATION_CTRL_ChangeState(CTRL_STATE_IDLE);
            break;
    }
}

/**
 * @brief  Read Sensors via SENSORS driver
 */
void IRRIGATION_CTRL_ReadSensors(void) {
    ctrl.ph_data.ph_value = PH_GetValue();
    ctrl.ec_data.ec_value = EC_GetValue();
    ctrl.ph_data.status = PH_GetStatus();
    ctrl.ec_data.status = EC_GetStatus();
}

float IRRIGATION_CTRL_GetPH(void) {
    return PH_GetValue();
}

float IRRIGATION_CTRL_GetEC(void) {
    return EC_GetValue();
}

uint8_t IRRIGATION_CTRL_IsPHValid(void) {
    return (PH_GetStatus() == SENSOR_OK);
}

uint8_t IRRIGATION_CTRL_IsECValid(void) {
    return (EC_GetStatus() == SENSOR_OK);
}

uint8_t IRRIGATION_CTRL_CheckPH(void) {
    return IRRIGATION_CTRL_IsPHInRange();
}

uint8_t IRRIGATION_CTRL_CheckEC(void) {
    return IRRIGATION_CTRL_IsECInRange();
}

/**
 * @brief  Check if pH is within target ± hysteresis
 */
uint8_t IRRIGATION_CTRL_IsPHInRange(void) {
    float diff = ctrl.ph_data.ph_value - ctrl.ph_params.target;
    return (fabsf(diff) <= ctrl.ph_params.hysteresis);
}

/**
 * @brief  Check if EC is within target ± hysteresis
 */
uint8_t IRRIGATION_CTRL_IsECInRange(void) {
    float diff = ctrl.ec_data.ec_value - ctrl.ec_params.target;
    return (fabsf(diff) <= ctrl.ec_params.hysteresis);
}

/**
 * @brief  Adjust pH (Dose acid or base)
 */
void IRRIGATION_CTRL_AdjustPH(void) {
    uint8_t valve_id = 0U;

    if (ctrl.ph_data.ph_value > ctrl.ph_params.target) {
        valve_id = ctrl.ph_params.acid_valve_id;
    } else if (ctrl.ph_data.ph_value < ctrl.ph_params.target) {
        valve_id = ctrl.ph_params.base_valve_id;
    }

    if (valve_id == 0U) {
        IRRIGATION_CTRL_StartMixing();
        return;
    }

    IRRIGATION_CTRL_UpdateDoseState(valve_id, ctrl.ph_params.dose_time_ms);
}

/**
 * @brief  Adjust EC (Dose fertilizer)
 */
void IRRIGATION_CTRL_AdjustEC(void) {
    if (ctrl.ec_data.ec_value < ctrl.ec_params.target) {
        IRRIGATION_CTRL_UpdateDoseState(ctrl.ec_params.fertilizer_valve_id,
                                        ctrl.ec_params.dose_time_ms);
    } else {
        IRRIGATION_CTRL_StartMixing();
    }
}

/**
 * @brief  Start watering next parcel in queue
 */
void IRRIGATION_CTRL_NextParcel(void) {
    while (ctrl.queue_size > 0U) {
        uint8_t parcel_id = ctrl.parcel_queue[ctrl.queue_head];

        ctrl.parcel_queue[ctrl.queue_head] = 0U;
        ctrl.queue_head = (ctrl.queue_head + 1U) % VALVE_COUNT;
        ctrl.queue_size--;

        if (parcel_id == 0U || PARCELS_IsEnabled(parcel_id) == 0U) {
            continue;
        }

        ctrl.current_parcel.parcel_id = parcel_id;
        ctrl.current_parcel.valve_id = parcel_id; /* Simple mapping */
        ctrl.current_parcel.start_time = HAL_GetTick();
        ctrl.current_parcel.duration_sec = PARCELS_GetDuration(parcel_id);
        ctrl.current_parcel.elapsed_sec = 0U;
        ctrl.current_parcel.is_complete = 0U;
        ctrl.current_parcel.is_skipped = 0U;

        VALVES_Open(ctrl.current_parcel.valve_id);
        return;
    }

    IRRIGATION_CTRL_ResetCurrentParcel();
}

uint8_t IRRIGATION_CTRL_IsParcelComplete(void) {
    if (ctrl.current_parcel.parcel_id == 0) return 1;
    
    uint32_t elapsed = (HAL_GetTick() - ctrl.current_parcel.start_time) / 1000;
    ctrl.current_parcel.elapsed_sec = elapsed;
    if (elapsed >= ctrl.current_parcel.duration_sec) {
        ctrl.current_parcel.is_complete = 1U;
        ctrl.total_watering_time += ctrl.current_parcel.duration_sec;
        return 1;
    }
    return 0;
}

void IRRIGATION_CTRL_Start(void) {
    if (ctrl.state == CTRL_STATE_ERROR || ctrl.state == CTRL_STATE_EMERGENCY_STOP) {
        IRRIGATION_CTRL_ClearAllErrors();
        IRRIGATION_CTRL_ChangeState(CTRL_STATE_IDLE);
    }

    if (ctrl.queue_size == 0U) {
        for (uint8_t parcel_id = 1U; parcel_id <= VALVE_COUNT; parcel_id++) {
            if (PARCELS_IsEnabled(parcel_id) != 0U) {
                IRRIGATION_CTRL_AddToQueue(parcel_id);
            }
        }
    }

    if (ctrl.queue_size == 0U) {
        return;
    }

    ctrl.is_running = 1;
    ctrl.is_paused = 0;
    if (ctrl.state == CTRL_STATE_IDLE) {
        IRRIGATION_CTRL_ChangeState(CTRL_STATE_IDLE);
    }
}

void IRRIGATION_CTRL_Stop(void) {
    ctrl.is_running = 0;
    ctrl.is_paused = 0;
    IRRIGATION_CTRL_CompleteDoseIfNeeded();
    IRRIGATION_CTRL_ResetCurrentParcel();
    IRRIGATION_CTRL_ClearQueue();
    IRRIGATION_CTRL_StopMixing();
    ctrl.timers.settling_start_time = 0U;
    IRRIGATION_CTRL_ChangeState(CTRL_STATE_IDLE);
    VALVES_CloseAll();
}

void IRRIGATION_CTRL_Pause(void) {
    ctrl.is_paused = 1;
}

void IRRIGATION_CTRL_Resume(void) {
    ctrl.is_paused = 0;
}

uint8_t IRRIGATION_CTRL_IsRunning(void) {
    return ctrl.is_running;
}

void IRRIGATION_CTRL_SetAutoMode(auto_mode_t mode) {
    g_auto_mode = mode;
}

auto_mode_t IRRIGATION_CTRL_GetAutoMode(void) {
    return g_auto_mode;
}

control_state_t IRRIGATION_CTRL_GetState(void) {
    return ctrl.state;
}

const char *IRRIGATION_CTRL_GetStateName(control_state_t state) {
    switch (state) {
        case CTRL_STATE_IDLE:
            return "IDLE";
        case CTRL_STATE_CHECKING_SENSORS:
            return "CHECK";
        case CTRL_STATE_PH_ADJUSTING:
            return "PH";
        case CTRL_STATE_EC_ADJUSTING:
            return "EC";
        case CTRL_STATE_MIXING:
            return "MIX";
        case CTRL_STATE_SETTLING:
            return "SETTLE";
        case CTRL_STATE_PARCEL_WATERING:
            return "WATER";
        case CTRL_STATE_WAITING:
            return "WAIT";
        case CTRL_STATE_ERROR:
            return "ERROR";
        case CTRL_STATE_EMERGENCY_STOP:
            return "STOP";
        default:
            return "UNKNOWN";
    }
}

void IRRIGATION_CTRL_StartParcel(uint8_t parcel_id) {
    IRRIGATION_CTRL_ClearQueue();
    IRRIGATION_CTRL_AddToQueue(parcel_id);
    ctrl.current_parcel.parcel_id = 0U;
    ctrl.current_parcel.valve_id = 0U;
    IRRIGATION_CTRL_Start();
}

void IRRIGATION_CTRL_StopParcel(void) {
    if (ctrl.current_parcel.valve_id != 0U) {
        VALVES_Close(ctrl.current_parcel.valve_id);
    }

    IRRIGATION_CTRL_ResetCurrentParcel();
}

uint8_t IRRIGATION_CTRL_GetCurrentParcelId(void) {
    return ctrl.current_parcel.parcel_id;
}

uint32_t IRRIGATION_CTRL_GetRemainingTime(void) {
    uint32_t elapsed;

    if (!ctrl.is_running || ctrl.current_parcel.parcel_id == 0 ||
        ctrl.current_parcel.duration_sec == 0) {
        return 0;
    }

    elapsed = (HAL_GetTick() - ctrl.current_parcel.start_time) / 1000U;
    if (elapsed >= ctrl.current_parcel.duration_sec) {
        return 0;
    }

    return ctrl.current_parcel.duration_sec - elapsed;
}

void IRRIGATION_CTRL_AddToQueue(uint8_t parcel_id) {
    if (parcel_id == 0U || parcel_id > VALVE_COUNT || ctrl.queue_size >= VALVE_COUNT) {
        return;
    }

    if (PARCELS_IsEnabled(parcel_id) == 0U || IRRIGATION_CTRL_IsParcelQueued(parcel_id) != 0U ||
        ctrl.current_parcel.parcel_id == parcel_id) {
        return;
    }

    ctrl.parcel_queue[ctrl.queue_tail] = parcel_id;
    ctrl.queue_tail = (ctrl.queue_tail + 1U) % VALVE_COUNT;
    ctrl.queue_size++;
}

void IRRIGATION_CTRL_ClearQueue(void) {
    memset(ctrl.parcel_queue, 0, sizeof(ctrl.parcel_queue));
    ctrl.queue_head = 0U;
    ctrl.queue_tail = 0U;
    ctrl.queue_size = 0U;
}

void IRRIGATION_CTRL_StartMixing(void) {
    IRRIGATION_CTRL_CompleteDoseIfNeeded();
    ctrl.timers.mixing_start_time = HAL_GetTick();
    IRRIGATION_CTRL_ChangeState(CTRL_STATE_MIXING);
}

void IRRIGATION_CTRL_StopMixing(void) {
    ctrl.timers.mixing_start_time = 0U;
}

uint8_t IRRIGATION_CTRL_IsMixingComplete(void) {
    return ((HAL_GetTick() - ctrl.timers.mixing_start_time) >=
            (IRRIGATION_MIXING_TIME * 1000U)) ? 1U : 0U;
}

void IRRIGATION_CTRL_StartSettling(void) {
    IRRIGATION_CTRL_StopMixing();
    ctrl.timers.settling_start_time = HAL_GetTick();
    IRRIGATION_CTRL_ChangeState(CTRL_STATE_SETTLING);
}

uint8_t IRRIGATION_CTRL_IsSettlingComplete(void) {
    return ((HAL_GetTick() - ctrl.timers.settling_start_time) >=
            (IRRIGATION_SETTLING_TIME * 1000U)) ? 1U : 0U;
}

void IRRIGATION_CTRL_SetError(error_code_t error) {
    if (error == ERR_NONE) {
        return;
    }

    ctrl.last_error = error;
    ctrl.error_flags = 1U;
    ctrl.is_paused = 0U;
    ctrl.is_running = 0U;
    IRRIGATION_CTRL_CompleteDoseIfNeeded();
    IRRIGATION_CTRL_ResetCurrentParcel();
    IRRIGATION_CTRL_ClearQueue();
    IRRIGATION_CTRL_ChangeState(CTRL_STATE_ERROR);
    IRRIGATION_CTRL_OnError(error);
}

void IRRIGATION_CTRL_ClearError(error_code_t error) {
    if (ctrl.last_error == error) {
        ctrl.last_error = ERR_NONE;
    }

    if (ctrl.last_error == ERR_NONE) {
        ctrl.error_flags = 0U;
    }
}

void IRRIGATION_CTRL_ClearAllErrors(void) {
    ctrl.error_flags = 0U;
    ctrl.last_error = ERR_NONE;
}

error_code_t IRRIGATION_CTRL_GetLastError(void) {
    return ctrl.last_error;
}

uint8_t IRRIGATION_CTRL_HasErrors(void) {
    return (ctrl.error_flags != 0U) ? 1U : 0U;
}

const char *IRRIGATION_CTRL_GetErrorString(error_code_t error) {
    switch (error) {
        case ERR_NONE:
            return "NONE";
        case ERR_PH_SENSOR_FAULT:
            return "PH_SENSOR";
        case ERR_EC_SENSOR_FAULT:
            return "EC_SENSOR";
        case ERR_PH_OUT_OF_RANGE:
            return "PH_RANGE";
        case ERR_EC_OUT_OF_RANGE:
            return "EC_RANGE";
        case ERR_VALVE_STUCK:
            return "VALVE";
        case ERR_TIMEOUT:
            return "TIMEOUT";
        case ERR_LOW_WATER_LEVEL:
            return "LOW_WATER";
        case ERR_OVERCURRENT:
            return "OVERCURRENT";
        case ERR_COMMUNICATION:
            return "COMM";
        default:
            return "UNKNOWN";
    }
}

void IRRIGATION_CTRL_EmergencyStop(void) {
    IRRIGATION_CTRL_CompleteDoseIfNeeded();
    IRRIGATION_CTRL_ResetCurrentParcel();
    IRRIGATION_CTRL_ClearQueue();
    ctrl.error_flags = 1U;
    ctrl.last_error = ERR_TIMEOUT;
    IRRIGATION_CTRL_ChangeState(CTRL_STATE_EMERGENCY_STOP);
    ctrl.is_running = 0;
    VALVES_EmergencyStop();
}

uint32_t IRRIGATION_CTRL_GetTotalWateringTime(void) {
    return ctrl.total_watering_time;
}

uint32_t IRRIGATION_CTRL_GetTotalParcelCount(void) {
    return ctrl.total_parcel_count;
}

uint32_t IRRIGATION_CTRL_GetCurrentRunTime(void) {
    if (ctrl.current_parcel.parcel_id == 0U) {
        return 0U;
    }

    return (HAL_GetTick() - ctrl.current_parcel.start_time) / 1000U;
}

void IRRIGATION_CTRL_ResetStatistics(void) {
    ctrl.total_watering_time = 0U;
    ctrl.total_parcel_count = 0U;
}

void IRRIGATION_CTRL_SetSchedule(uint8_t slot, irrigation_schedule_t *sched) {
    if (slot >= VALVE_COUNT || sched == NULL) {
        return;
    }

    g_schedules[slot] = *sched;
}

void IRRIGATION_CTRL_GetSchedule(uint8_t slot, irrigation_schedule_t *sched) {
    if (slot >= VALVE_COUNT || sched == NULL) {
        return;
    }

    *sched = g_schedules[slot];
}

void IRRIGATION_CTRL_CheckSchedules(void) {
}

void IRRIGATION_CTRL_OnStateChange(control_state_t old_state,
                                   control_state_t new_state) {
    (void)old_state;
    (void)new_state;
}

void IRRIGATION_CTRL_OnPHAdjusted(float new_ph) {
    (void)new_ph;
}

void IRRIGATION_CTRL_OnECAdjusted(float new_ec) {
    (void)new_ec;
}

void IRRIGATION_CTRL_OnParcelComplete(uint8_t parcel_id) {
    (void)parcel_id;
}

void IRRIGATION_CTRL_OnError(error_code_t error) {
    (void)error;
}

static void IRRIGATION_CTRL_ChangeState(control_state_t new_state) {
    control_state_t old_state = ctrl.state;

    ctrl.state = new_state;
    ctrl.timers.state_entry_time = HAL_GetTick();
    if (old_state != new_state) {
        IRRIGATION_CTRL_OnStateChange(old_state, new_state);
    }
}

static void IRRIGATION_CTRL_CompleteDoseIfNeeded(void) {
    if (g_active_dosing_valve != 0U) {
        VALVES_Close(g_active_dosing_valve);
        g_active_dosing_valve = 0U;
    }
}

static void IRRIGATION_CTRL_UpdateDoseState(uint8_t valve_id, uint32_t dose_time_ms) {
    if (valve_id == 0U) {
        IRRIGATION_CTRL_StartMixing();
        return;
    }

    if (g_active_dosing_valve == 0U) {
        g_active_dosing_valve = valve_id;
        VALVES_Open(g_active_dosing_valve);
        ctrl.timers.state_entry_time = HAL_GetTick();
        return;
    }

    if (g_active_dosing_valve != valve_id) {
        IRRIGATION_CTRL_CompleteDoseIfNeeded();
        g_active_dosing_valve = valve_id;
        VALVES_Open(g_active_dosing_valve);
        ctrl.timers.state_entry_time = HAL_GetTick();
        return;
    }

    if ((HAL_GetTick() - ctrl.timers.state_entry_time) >= dose_time_ms) {
        IRRIGATION_CTRL_CompleteDoseIfNeeded();
        if (ctrl.state == CTRL_STATE_PH_ADJUSTING) {
            IRRIGATION_CTRL_OnPHAdjusted(ctrl.ph_data.ph_value);
        } else if (ctrl.state == CTRL_STATE_EC_ADJUSTING) {
            IRRIGATION_CTRL_OnECAdjusted(ctrl.ec_data.ec_value);
        }
        IRRIGATION_CTRL_StartMixing();
    }
}

static void IRRIGATION_CTRL_ResetCurrentParcel(void) {
    ctrl.current_parcel.parcel_id = 0U;
    ctrl.current_parcel.valve_id = 0U;
    ctrl.current_parcel.start_time = 0U;
    ctrl.current_parcel.duration_sec = 0U;
    ctrl.current_parcel.elapsed_sec = 0U;
    ctrl.current_parcel.is_complete = 0U;
    ctrl.current_parcel.is_skipped = 0U;
}

static uint8_t IRRIGATION_CTRL_IsParcelQueued(uint8_t parcel_id) {
    uint8_t index = ctrl.queue_head;
    uint8_t remaining = ctrl.queue_size;

    while (remaining > 0U) {
        if (ctrl.parcel_queue[index] == parcel_id) {
            return 1U;
        }

        index = (index + 1U) % VALVE_COUNT;
        remaining--;
    }

    return 0U;
}
