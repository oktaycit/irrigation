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
            ctrl.state = CTRL_STATE_CHECKING_SENSORS;
            ctrl.timers.state_entry_time = HAL_GetTick();
            break;

        case CTRL_STATE_CHECKING_SENSORS:
            IRRIGATION_CTRL_ReadSensors();
            if (IRRIGATION_CTRL_IsPHInRange() && IRRIGATION_CTRL_IsECInRange()) {
                ctrl.state = CTRL_STATE_PARCEL_WATERING;
            } else if (!IRRIGATION_CTRL_IsPHInRange()) {
                ctrl.state = CTRL_STATE_PH_ADJUSTING;
            } else {
                ctrl.state = CTRL_STATE_EC_ADJUSTING;
            }
            ctrl.timers.state_entry_time = HAL_GetTick();
            break;

        case CTRL_STATE_PH_ADJUSTING:
            IRRIGATION_CTRL_AdjustPH();
            /* After dosing, wait/mix */
            ctrl.state = CTRL_STATE_MIXING;
            ctrl.timers.state_entry_time = HAL_GetTick();
            break;

        case CTRL_STATE_EC_ADJUSTING:
            IRRIGATION_CTRL_AdjustEC();
            ctrl.state = CTRL_STATE_MIXING;
            ctrl.timers.state_entry_time = HAL_GetTick();
            break;

        case CTRL_STATE_MIXING:
            /* Start mixing pump/valve if any */
            if ((HAL_GetTick() - ctrl.timers.state_entry_time) / 1000 >= IRRIGATION_MIXING_TIME) {
                ctrl.state = CTRL_STATE_SETTLING;
                ctrl.timers.state_entry_time = HAL_GetTick();
            }
            break;

        case CTRL_STATE_SETTLING:
            if ((HAL_GetTick() - ctrl.timers.state_entry_time) / 1000 >= IRRIGATION_SETTLING_TIME) {
                ctrl.state = CTRL_STATE_CHECKING_SENSORS; /* Re-check values */
            }
            break;

        case CTRL_STATE_PARCEL_WATERING:
            if (ctrl.current_parcel.parcel_id == 0) {
                /* Start first available parcel from queue or sequence */
                IRRIGATION_CTRL_NextParcel();
            }
            
            if (IRRIGATION_CTRL_IsParcelComplete()) {
                VALVES_Close(ctrl.current_parcel.valve_id);
                if (ctrl.queue_size > 0) {
                    IRRIGATION_CTRL_NextParcel();
                } else {
                    ctrl.state = CTRL_STATE_IDLE;
                    ctrl.is_running = 0;
                }
            }
            break;

        case CTRL_STATE_ERROR:
            VALVES_EmergencyStop();
            break;

        default:
            ctrl.state = CTRL_STATE_IDLE;
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
    return (fabsf(diff) <= IRRIGATION_PH_HYSTERESIS);
}

/**
 * @brief  Check if EC is within target ± hysteresis
 */
uint8_t IRRIGATION_CTRL_IsECInRange(void) {
    float diff = ctrl.ec_data.ec_value - ctrl.ec_params.target;
    return (fabsf(diff) <= IRRIGATION_EC_HYSTERESIS);
}

/**
 * @brief  Adjust pH (Dose acid or base)
 */
void IRRIGATION_CTRL_AdjustPH(void) {
    if (ctrl.ph_data.ph_value > ctrl.ph_params.target) {
        /* Need acid */
        VALVES_Open(ctrl.ph_params.acid_valve_id);
        HAL_Delay(ctrl.ph_params.dose_time_ms);
        VALVES_Close(ctrl.ph_params.acid_valve_id);
    }
    /* Base adjustment could be implemented here as well */
}

/**
 * @brief  Adjust EC (Dose fertilizer)
 */
void IRRIGATION_CTRL_AdjustEC(void) {
    if (ctrl.ec_data.ec_value < ctrl.ec_params.target) {
        /* Need fertilizer */
        VALVES_Open(ctrl.ec_params.fertilizer_valve_id);
        HAL_Delay(ctrl.ec_params.dose_time_ms);
        VALVES_Close(ctrl.ec_params.fertilizer_valve_id);
    }
}

/**
 * @brief  Start watering next parcel in queue
 */
void IRRIGATION_CTRL_NextParcel(void) {
    if (ctrl.queue_size == 0) return;

    ctrl.current_parcel.parcel_id = ctrl.parcel_queue[ctrl.queue_head];
    ctrl.current_parcel.valve_id = ctrl.current_parcel.parcel_id; /* Simple mapping */
    ctrl.current_parcel.start_time = HAL_GetTick();
    ctrl.current_parcel.duration_sec = PARCELS_GetDuration(ctrl.current_parcel.parcel_id);
    ctrl.current_parcel.is_complete = 0;

    VALVES_Open(ctrl.current_parcel.valve_id);

    /* Update queue */
    ctrl.queue_head = (ctrl.queue_head + 1) % 8;
    ctrl.queue_size--;
}

uint8_t IRRIGATION_CTRL_IsParcelComplete(void) {
    if (ctrl.current_parcel.parcel_id == 0) return 1;
    
    uint32_t elapsed = (HAL_GetTick() - ctrl.current_parcel.start_time) / 1000;
    if (elapsed >= ctrl.current_parcel.duration_sec) {
        return 1;
    }
    return 0;
}

void IRRIGATION_CTRL_Start(void) {
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
        ctrl.timers.state_entry_time = HAL_GetTick();
    }
}

void IRRIGATION_CTRL_Stop(void) {
    ctrl.is_running = 0;
    ctrl.is_paused = 0;
    ctrl.state = CTRL_STATE_IDLE;
    ctrl.current_parcel.parcel_id = 0;
    ctrl.current_parcel.valve_id = 0;
    ctrl.current_parcel.duration_sec = 0U;
    IRRIGATION_CTRL_ClearQueue();
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

    ctrl.current_parcel.parcel_id = 0U;
    ctrl.current_parcel.valve_id = 0U;
    ctrl.current_parcel.duration_sec = 0U;
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

void IRRIGATION_CTRL_EmergencyStop(void) {
    ctrl.state = CTRL_STATE_EMERGENCY_STOP;
    ctrl.is_running = 0;
    VALVES_EmergencyStop();
}
