/**
 ******************************************************************************
 * @file           : eeprom.c
 * @brief          : EEPROM 24C256 Veri Saklama Driver Implementation
 ******************************************************************************
 */

#include "main.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;

/* Internal Handle */
static eeprom_handle_t h_eeprom = {0};

/**
 * @brief  Initialize EEPROM
 */
uint8_t EEPROM_Init(void) {
    /* Check if EEPROM is ready */
    if (HAL_I2C_IsDeviceReady(&hi2c1, (EEPROM_ADDRESS << 1), 3, 100) != HAL_OK) {
        h_eeprom.last_error = EEPROM_NOT_INITIALIZED;
        return EEPROM_ERROR;
    }

    /* Load Header */
    if (EEPROM_ReadBuffer(EEPROM_ADDR_MAGIC, (uint8_t*)&h_eeprom.header, sizeof(eeprom_header_t)) != EEPROM_OK) {
        return EEPROM_ERROR;
    }

    /* Check Magic Number */
    if (h_eeprom.header.magic != EEPROM_MAGIC_VALUE) {
        /* Format or initialize default values */
        EEPROM_Format();
    }

    h_eeprom.initialized = 1;
    return EEPROM_OK;
}

/**
 * @brief  Read buffer from EEPROM
 */
uint8_t EEPROM_ReadBuffer(uint16_t address, uint8_t *buffer, uint16_t size) {
    if (HAL_I2C_Mem_Read(&hi2c1, (EEPROM_ADDRESS << 1), address, I2C_MEMADD_SIZE_16BIT, buffer, size, 1000) != HAL_OK) {
        h_eeprom.last_error = EEPROM_READ_ERROR;
        return EEPROM_ERROR;
    }
    return EEPROM_OK;
}

/**
 * @brief  Write buffer to EEPROM (Handles page crossing)
 */
uint8_t EEPROM_WriteBuffer(uint16_t address, uint8_t *buffer, uint16_t size) {
    uint16_t remaining = size;
    uint16_t current_addr = address;
    uint8_t *p_buffer = buffer;

    while (remaining > 0) {
        /* Calculate how many bytes can be written in current page */
        uint16_t page_offset = current_addr % EEPROM_PAGE_SIZE;
        uint16_t to_write = EEPROM_PAGE_SIZE - page_offset;
        
        if (to_write > remaining) to_write = remaining;

        if (HAL_I2C_Mem_Write(&hi2c1, (EEPROM_ADDRESS << 1), current_addr, I2C_MEMADD_SIZE_16BIT, p_buffer, to_write, 1000) != HAL_OK) {
            h_eeprom.last_error = EEPROM_WRITE_ERROR;
            return EEPROM_ERROR;
        }

        /* Wait for write cycle completion (max 5ms for 24C256) */
        HAL_Delay(5);

        remaining -= to_write;
        current_addr += to_write;
        p_buffer += to_write;
    }

    return EEPROM_OK;
}

/**
 * @brief  Load System Parameters
 */
uint8_t EEPROM_LoadSystemParams(void) {
    return EEPROM_ReadBuffer(EEPROM_ADDR_SYSTEM, (uint8_t*)&h_eeprom.system, sizeof(eeprom_system_t));
}

/**
 * @brief  Save System Parameters
 */
uint8_t EEPROM_SaveSystemParams(void) {
    /* Update CRC before saving */
    h_eeprom.system.crc = EEPROM_CalculateCRC(&h_eeprom.system, sizeof(eeprom_system_t) - 2);
    return EEPROM_WriteBuffer(EEPROM_ADDR_SYSTEM, (uint8_t*)&h_eeprom.system, sizeof(eeprom_system_t));
}

/**
 * @brief  Format EEPROM with default values
 */
void EEPROM_Format(void) {
    memset(&h_eeprom, 0, sizeof(eeprom_handle_t));
    
    h_eeprom.header.magic = EEPROM_MAGIC_VALUE;
    h_eeprom.header.version_major = FIRMWARE_VERSION_MAJOR;
    h_eeprom.header.version_minor = FIRMWARE_VERSION_MINOR;
    h_eeprom.header.initialized = 1;

    /* Write Header */
    EEPROM_WriteBuffer(EEPROM_ADDR_MAGIC, (uint8_t*)&h_eeprom.header, sizeof(eeprom_header_t));

    /* Initialize default system params */
    h_eeprom.system.ph_target = 6.5f;
    h_eeprom.system.ph_min = 5.5f;
    h_eeprom.system.ph_max = 7.5f;
    h_eeprom.system.ec_target = 1.8f;
    h_eeprom.system.ec_min = 1.0f;
    h_eeprom.system.ec_max = 2.5f;
    h_eeprom.system.brightness = 80;
    
    EEPROM_SaveSystemParams();
}

/**
 * @brief  Calculate 16-bit CRC
 */
uint16_t EEPROM_CalculateCRC(void *data, uint16_t size) {
    uint16_t crc = 0xFFFF;
    uint8_t *p = (uint8_t*)data;

    for (uint16_t i = 0; i < size; i++) {
        crc ^= p[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

/* Other functions (Logs, Calibration load/save) would follow same pattern... */
uint8_t EEPROM_GetLastError(void) { return h_eeprom.last_error; }
