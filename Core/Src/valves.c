/**
 ******************************************************************************
 * @file           : valves.c
 * @brief          : 8 Kanal Vana Kontrol Driver Implementation
 ******************************************************************************
 */

#include "main.h"
#include <string.h>
#include <stdio.h>

/* Global Valve Data */
static valve_t g_valves[VALVE_COUNT];
static parcel_t g_parcels[VALVE_COUNT];
static irrigation_status_t g_irrigation_status = {0};

/* Private Function Prototypes */
static void VALVES_UpdateState(uint8_t id, valve_state_t new_state);

/**
 * @brief  Initialize Valves and Parcels
 */
void VALVES_Init(void) {
    memset(g_valves, 0, sizeof(g_valves));
    memset(g_parcels, 0, sizeof(g_parcels));

    for (uint8_t i = 0; i < VALVE_COUNT; i++) {
        g_valves[i].id = i + 1;
        g_valves[i].state = VALVE_STATE_CLOSED;
        g_valves[i].mode = VALVE_MODE_AUTO;
        
        g_parcels[i].parcel_id = i + 1;
        g_parcels[i].valve_id = i + 1;
        g_parcels[i].enabled = 1;
        snprintf(g_parcels[i].name, 16, "Parsel %d", i + 1);
        g_parcels[i].irrigation_duration_sec = 300; /* 5 minutes default */
    }
}

/**
 * @brief  Open a specific valve
 */
void VALVES_Open(uint8_t valve_id) {
    if (valve_id == 0 || valve_id > VALVE_COUNT) return;
    uint8_t idx = valve_id - 1;

    uint16_t pins[] = {VALVE_1_PIN, VALVE_2_PIN, VALVE_3_PIN, VALVE_4_PIN,
                       VALVE_5_PIN, VALVE_6_PIN, VALVE_7_PIN, VALVE_8_PIN};

    HAL_GPIO_WritePin(VALVE_PORT, pins[idx], GPIO_PIN_SET);
    VALVES_UpdateState(valve_id, VALVE_STATE_OPEN);
    
    g_valves[idx].cycle_count++;
    g_valves[idx].last_action_time = HAL_GetTick();
}

/**
 * @brief  Close a specific valve
 */
void VALVES_Close(uint8_t valve_id) {
    if (valve_id == 0 || valve_id > VALVE_COUNT) return;
    uint8_t idx = valve_id - 1;

    uint16_t pins[] = {VALVE_1_PIN, VALVE_2_PIN, VALVE_3_PIN, VALVE_4_PIN,
                       VALVE_5_PIN, VALVE_6_PIN, VALVE_7_PIN, VALVE_8_PIN};

    HAL_GPIO_WritePin(VALVE_PORT, pins[idx], GPIO_PIN_RESET);
    VALVES_UpdateState(valve_id, VALVE_STATE_CLOSED);

    if (g_valves[idx].last_action_time > 0) {
        uint32_t duration = (HAL_GetTick() - g_valves[idx].last_action_time) / 1000;
        g_valves[idx].total_on_time += duration;
    }
}

/**
 * @brief  Toggle a specific valve
 */
void VALVES_Toggle(uint8_t valve_id) {
    if (VALVES_GetState(valve_id) == VALVE_STATE_OPEN) {
        VALVES_Close(valve_id);
    } else {
        VALVES_Open(valve_id);
    }
}

/**
 * @brief  Get current state of a valve
 */
uint8_t VALVES_GetState(uint8_t valve_id) {
    if (valve_id == 0 || valve_id > VALVE_COUNT) return VALVE_STATE_ERROR;
    return g_valves[valve_id - 1].state;
}

void VALVES_SetMode(uint8_t valve_id, valve_mode_t mode) {
    if (valve_id == 0 || valve_id > VALVE_COUNT) return;
    g_valves[valve_id - 1].mode = mode;
}

valve_mode_t VALVES_GetMode(uint8_t valve_id) {
    if (valve_id == 0 || valve_id > VALVE_COUNT) return VALVE_MODE_DISABLED;
    return g_valves[valve_id - 1].mode;
}

void VALVES_ManualOpen(uint8_t valve_id) {
    VALVES_SetMode(valve_id, VALVE_MODE_MANUAL);
    VALVES_Open(valve_id);
}

void VALVES_ManualClose(uint8_t valve_id) {
    VALVES_Close(valve_id);
}

uint8_t VALVES_IsManualMode(uint8_t valve_id) {
    if (valve_id == 0 || valve_id > VALVE_COUNT) return 0;
    return (g_valves[valve_id - 1].mode == VALVE_MODE_MANUAL);
}

/**
 * @brief  Emergency stop for all valves
 */
void VALVES_EmergencyStop(void) {
    VALVES_CloseAll();
}

/**
 * @brief  Close all valves
 */
void VALVES_CloseAll(void) {
    for (uint8_t i = 1; i <= VALVE_COUNT; i++) {
        VALVES_Close(i);
    }
}

/**
 * @brief  Open all valves
 */
void VALVES_OpenAll(void) {
    for (uint8_t i = 1; i <= VALVE_COUNT; i++) {
        VALVES_Open(i);
    }
}

uint16_t VALVES_GetActiveMask(void) {
    uint16_t mask = 0U;

    for (uint8_t i = 0U; i < VALVE_COUNT; i++) {
        if (g_valves[i].state == VALVE_STATE_OPEN) {
            mask |= (uint16_t)(1U << i);
        }
    }

    return mask;
}

void VALVES_SetActiveMask(uint16_t mask) {
    for (uint8_t i = 0U; i < VALVE_COUNT; i++) {
        if ((mask & (1U << i)) != 0U) {
            VALVES_Open(i + 1U);
        } else {
            VALVES_Close(i + 1U);
        }
    }
}

void VALVES_Update(void) {
}

/**
 * @brief  Internal state update helper
 */
static void VALVES_UpdateState(uint8_t id, valve_state_t new_state) {
    if (id == 0 || id > VALVE_COUNT) return;
    g_valves[id - 1].state = new_state;
}

/* Parcel Management */
void PARCELS_Init(void) {
    /* Already partially handled in VALVES_Init */
}

void PARCELS_SetDuration(uint8_t parcel_id, uint32_t duration_sec) {
    if (parcel_id == 0 || parcel_id > VALVE_COUNT) return;
    g_parcels[parcel_id - 1].irrigation_duration_sec = duration_sec;
}

uint32_t PARCELS_GetDuration(uint8_t parcel_id) {
    if (parcel_id == 0 || parcel_id > VALVE_COUNT) return 0;
    return g_parcels[parcel_id - 1].irrigation_duration_sec;
}

void PARCELS_SetEnabled(uint8_t parcel_id, uint8_t enabled) {
    if (parcel_id == 0 || parcel_id > VALVE_COUNT) return;
    g_parcels[parcel_id - 1].enabled = (enabled != 0U) ? 1U : 0U;
}

uint8_t PARCELS_IsEnabled(uint8_t parcel_id) {
    if (parcel_id == 0 || parcel_id > VALVE_COUNT) return 0;
    return g_parcels[parcel_id - 1].enabled;
}

void PARCELS_SetName(uint8_t parcel_id, const char *name) {
    if (parcel_id == 0 || parcel_id > VALVE_COUNT || name == NULL) return;

    snprintf(g_parcels[parcel_id - 1].name, sizeof(g_parcels[parcel_id - 1].name),
             "%s", name);
}

const char *PARCELS_GetName(uint8_t parcel_id) {
    static const char *empty_name = "";

    if (parcel_id == 0 || parcel_id > VALVE_COUNT) return empty_name;
    return g_parcels[parcel_id - 1].name;
}

/* Irrigation Logic Implementation */
uint8_t IRRIGATION_IsRunning(void) {
    return g_irrigation_status.is_running;
}

void IRRIGATION_Update(void) {
    if (!g_irrigation_status.is_running) return;

    /* Per-second logic could go here, or handled by a timer interrupt */
    /* For now, just a placeholder for periodic update */
}

irrigation_status_t IRRIGATION_GetStatus(void) {
    return g_irrigation_status;
}

/* Valve Statistics */
uint32_t VALVES_GetTotalOnTime(uint8_t valve_id) {
    if (valve_id == 0 || valve_id > VALVE_COUNT) return 0;
    return g_valves[valve_id - 1].total_on_time;
}

uint32_t VALVES_GetCycleCount(uint8_t valve_id) {
    if (valve_id == 0 || valve_id > VALVE_COUNT) return 0;
    return g_valves[valve_id - 1].cycle_count;
}
