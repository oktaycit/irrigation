/**
 ******************************************************************************
 * @file           : system_config.c
 * @brief          : Sistem konfigürasyon ve durum yönetimi
 ******************************************************************************
 */

#include "main.h"
#include "irrigation_control.h"

SystemStatus_t gSystemStatus = {0};

/**
 * @brief  Global System Initialization
 */
void System_Init(void) {
    /* Initialize peripherals and drivers in correct order */
    gSystemStatus = (SystemStatus_t){0};
    
    /* 1. EEPROM (to load settings) */
    if (EEPROM_Init() == EEPROM_OK) {
        gSystemStatus.eeprom_ok = 1;
    }

    /* 2. LCD & Touch */
    LCD_Init();
    gSystemStatus.lcd_ok = 1;
    
    TOUCH_Init();
    gSystemStatus.touch_ok = 1;

    GUI_Init();
    LCD_SetBacklight(80);

    /* 3. Sensors */
    SENSORS_Init();
    SENSORS_StartContinuous();
    gSystemStatus.ph_sensor_ok = 1;
    gSystemStatus.ec_sensor_ok = 1;

    /* 4. Valves */
    VALVES_Init();

    /* 5. Irrigation Control */
    IRRIGATION_CTRL_Init();

    gSystemStatus.system_ready = 1;
}

/**
 * @brief  Periodic System Status Check
 */
void System_Status_Update(void) {
    /* Check for sensor timeouts or out of range values */
    if (PH_GetStatus() != SENSOR_OK) {
        gSystemStatus.ph_sensor_ok = 0;
    } else {
        gSystemStatus.ph_sensor_ok = 1;
    }

    if (EC_GetStatus() != SENSOR_OK) {
        gSystemStatus.ec_sensor_ok = 0;
    } else {
        gSystemStatus.ec_sensor_ok = 1;
    }

    /* Update global error code based on sensor/valve states */
    if (!gSystemStatus.ph_sensor_ok || !gSystemStatus.ec_sensor_ok) {
        gSystemStatus.error_code = 1; /* Sensor Error */
    } else {
        gSystemStatus.error_code = 0;
    }
}
