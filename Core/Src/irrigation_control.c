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

/**
 * @brief  Initialize Irrigation Controller
 */
void IRRIGATION_CTRL_Init(void) {
    memset(&ctrl, 0, sizeof(irrigation_control_t));
    ctrl.state = CTRL_STATE_IDLE;
    
    /* Load parameters from EEPROM */
    EEPROM_LoadSystemParams();
    /* Map EEPROM params to ctrl params */
    // ctrl.ph_params.target = h_eeprom.system.ph_target; ...
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
    ctrl.is_running = 1;
    ctrl.is_paused = 0;
}

void IRRIGATION_CTRL_Stop(void) {
    ctrl.is_running = 0;
    VALVES_CloseAll();
}

void IRRIGATION_CTRL_EmergencyStop(void) {
    ctrl.state = CTRL_STATE_EMERGENCY_STOP;
    ctrl.is_running = 0;
    VALVES_EmergencyStop();
}
