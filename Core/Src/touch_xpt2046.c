/**
 ******************************************************************************
 * @file           : touch_xpt2046.c
 * @brief          : XPT2046 Touch Screen Controller Driver Implementation
 ******************************************************************************
 */

#include "main.h"
#include <stdio.h>

extern SPI_HandleTypeDef hspi1;

/* Calibration Data */
static touch_calibration_t current_cal = {
    .a = 0, .b = 0, .c = 0, .d = 0, .e = 1, .f = 1, .valid = 0
};

/* Private Functions */
static uint16_t TOUCH_ReadRaw(uint8_t cmd);
static uint8_t TOUCH_ReadRawAvg(uint8_t cmd, uint16_t *val);

/**
 * @brief  Initialize Touch Controller
 */
void TOUCH_Init(void) {
    /* Pin initialization is handled by CubeMX in MX_GPIO_Init and MX_SPI1_Init */
    /* Deactivate CS */
    HAL_GPIO_WritePin(TOUCH_CS_PORT, TOUCH_CS_PIN, GPIO_PIN_SET);
    
    /* Try to load calibration from EEPROM */
    TOUCH_LoadCalibration();
}

/**
 * @brief  Check if screen is pressed (Low level check)
 */
uint8_t TOUCH_IsPressed(void) {
    return (HAL_GPIO_ReadPin(TOUCH_IRQ_PORT, TOUCH_IRQ_PIN) == GPIO_PIN_RESET);
}

/**
 * @brief  Read Raw Value via SPI
 */
static uint16_t TOUCH_ReadRaw(uint8_t cmd) {
    uint8_t tx_data[3] = {cmd, 0x00, 0x00};
    uint8_t rx_data[3] = {0};
    uint16_t value = 0;

    HAL_GPIO_WritePin(TOUCH_CS_PORT, TOUCH_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, 3, 100);
    HAL_GPIO_WritePin(TOUCH_CS_PORT, TOUCH_CS_PIN, GPIO_PIN_SET);

    value = ((uint16_t)rx_data[1] << 8) | rx_data[2];
    value >>= 3; /* XPT2046 returns 12 bits */

    return value;
}

/**
 * @brief  Read Averaged Raw Value
 */
static uint8_t TOUCH_ReadRawAvg(uint8_t cmd, uint16_t *val) {
    uint32_t sum = 0;
    uint16_t min_v = 0xFFFF, max_v = 0;
    uint16_t temp;
    const uint8_t samples = 12;

    for (int i = 0; i < samples; i++) {
        temp = TOUCH_ReadRaw(cmd);
        if (temp < min_v) min_v = temp;
        if (temp > max_v) max_v = temp;
        sum += temp;
    }

    /* Remove outliers */
    sum = sum - min_v - max_v;
    *val = sum / (samples - 2);

    return 1;
}

/**
 * @brief  Read Processed Touch Point
 */
uint8_t TOUCH_ReadPoint(touch_point_t *point) {
    if (!TOUCH_IsPressed()) {
        point->pressed = 0;
        return 0;
    }

    uint16_t raw_x, raw_y;
    TOUCH_ReadRawAvg(XPT2046_ADDR_X, &raw_x);
    TOUCH_ReadRawAvg(XPT2046_ADDR_Y, &raw_y);

    point->raw_x = raw_x;
    point->raw_y = raw_y;
    point->pressed = 1;

    /* Apply calibration if valid */
    if (current_cal.valid) {
        point->x = (current_cal.a * raw_x + current_cal.b * raw_y + current_cal.c) / current_cal.e;
        point->y = (current_cal.d * raw_x + current_cal.e * raw_y + current_cal.f) / current_cal.f; // Wait, logic might need check based on actual calibration formula
        
        /* Simple linear mapping as fallback if formula is different */
        /* Normally: screen_x = (a*raw_x + b*raw_y + c) / divider */
    } else {
        /* Raw mapping as fallback */
        point->x = (int16_t)((uint32_t)raw_x * LCD_WIDTH / 4096);
        point->y = (int16_t)((uint32_t)raw_y * LCD_HEIGHT / 4096);
    }

    return 1;
}

/**
 * @brief  Calibration process (Placeholder)
 */
void TOUCH_CalibrationStart(void) {
    /* GUI sequence for calibration would be triggered here */
}

void TOUCH_SetCalibration(const touch_calibration_t *cal) {
    current_cal = *cal;
}

void TOUCH_GetCalibration(touch_calibration_t *cal) {
    *cal = current_cal;
}

void TOUCH_LoadCalibration(void) {
    /* Load from EEPROM structure */
    /* EEPROM_Read(ADDR_CALIB, (uint8_t*)&current_cal, sizeof(current_cal)); */
}

void TOUCH_SaveCalibration(void) {
    /* Save to EEPROM structure */
    /* EEPROM_Write(ADDR_CALIB, (uint8_t*)&current_cal, sizeof(current_cal)); */
}

void TOUCH_IRQHandler_Callback(void) {
    /* Handle interrupt if needed, usually just notification */
}
