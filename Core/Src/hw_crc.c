/**
 ******************************************************************************
 * @file           : hw_crc.c
 * @brief          : Hardware CRC Driver Implementation (STM32F4 CRC peripheral)
 ******************************************************************************
 */

#include "main.h"
#include "hw_crc.h"

extern CRC_HandleTypeDef hcrc;
static uint8_t g_crc_initialized = 0;

/**
 * @brief  Initialize Hardware CRC
 */
uint8_t CRC_Init(void) {
    if (hcrc.Instance == CRC) {
        g_crc_initialized = 1;
        return 1;
    }
    return 0;
}

/**
 * @brief  Calculate CRC-32 for byte buffer using hardware peripheral
 * @param  data: Pointer to data buffer
 * @param  size: Size of data in bytes
 * @retval CRC-32 result
 */
uint32_t CRC_Calculate(const uint8_t *data, size_t size) {
    if (!g_crc_initialized || data == NULL || size == 0) {
        return 0;
    }
    
    /* Reset CRC peripheral */
    CRC_Reset();
    
    /* Process data in 32-bit words for maximum speed */
    size_t word_count = size / 4;
    size_t remaining_bytes = size % 4;
    
    const uint32_t *word_data = (const uint32_t *)data;
    
    /* Calculate CRC for full 32-bit words */
    if (word_count > 0) {
        HAL_CRC_Accumulate(&hcrc, word_data, word_count);
    }
    
    /* Handle remaining bytes */
    if (remaining_bytes > 0) {
        uint8_t last_word = 0;
        const uint8_t *byte_ptr = data + (word_count * 4);
        
        for (size_t i = 0; i < remaining_bytes; i++) {
            last_word |= (byte_ptr[i] << (i * 8));
        }
        
        HAL_CRC_Accumulate(&hcrc, &last_word, 1);
    }
    
    return hcrc.Instance->DR;
}

/**
 * @brief  Calculate CRC-32 for 32-bit word buffer
 * @param  data: Pointer to 32-bit word buffer
 * @param  size: Number of 32-bit words
 * @retval CRC-32 result
 */
uint32_t CRC_Calculate32(const uint32_t *data, size_t size) {
    if (!g_crc_initialized || data == NULL || size == 0) {
        return 0;
    }
    
    CRC_Reset();
    HAL_CRC_Accumulate(&hcrc, data, size);
    return hcrc.Instance->DR;
}

/**
 * @brief  Get current CRC result
 */
uint32_t CRC_GetResult(void) {
    return hcrc.Instance->DR;
}

/**
 * @brief  Reset CRC peripheral
 */
void CRC_Reset(void) {
    if (hcrc.Instance == CRC) {
        __HAL_CRC_DR_RESET(&hcrc);
    }
}
