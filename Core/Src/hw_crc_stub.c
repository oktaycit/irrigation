/**
 ******************************************************************************
 * @file           : hw_crc_stub.c
 * @brief          : Hardware CRC Driver Stub (HAL CRC driver not available)
 ******************************************************************************
 * @note           : Uses software CRC-32 implementation
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "hw_crc.h"

/* Private define ------------------------------------------------------------*/
/* CRC-32 Polynomial: 0x04C11DB7 (reversed: 0xEDB88320) */
#define CRC32_POLYNOMIAL 0xEDB88320UL

/* Private variables ---------------------------------------------------------*/
static uint32_t g_crc_result = 0xFFFFFFFFUL;
static uint8_t g_crc_initialized = 0;

/* Private functions ---------------------------------------------------------*/
/**
 * @brief Calculate CRC-32 of a single byte
 * @param crc Current CRC value
 * @param data Data byte
 * @retval Updated CRC value
 */
static uint32_t crc32_byte(uint32_t crc, uint8_t data)
{
  crc ^= data;
  for (int i = 0; i < 8; i++) {
    crc = (crc & 1) ? ((crc >> 1) ^ CRC32_POLYNOMIAL) : (crc >> 1);
  }
  return crc;
}

/**
 * @brief Initialize CRC (stub)
 * @retval 1 if success
 */
uint8_t CRC_Init(void)
{
  g_crc_result = 0xFFFFFFFFUL;
  g_crc_initialized = 1;
  return 1;
}

/**
 * @brief Calculate CRC-32
 * @param data Data buffer
 * @param size Buffer size
 * @retval CRC-32 value
 */
uint32_t CRC_Calculate(const uint8_t *data, size_t size)
{
  uint32_t crc = 0xFFFFFFFFUL;
  
  if (data == NULL || size == 0) {
    return 0;
  }
  
  for (size_t i = 0; i < size; i++) {
    crc = crc32_byte(crc, data[i]);
  }
  
  g_crc_result = crc ^ 0xFFFFFFFFUL;
  return g_crc_result;
}

/**
 * @brief Calculate CRC-32 for 32-bit data
 * @param data 32-bit data buffer
 * @param size Buffer size (number of 32-bit words)
 * @retval CRC-32 value
 */
uint32_t CRC_Calculate32(const uint32_t *data, size_t size)
{
  if (data == NULL || size == 0) {
    return 0;
  }
  
  /* Convert to byte array and calculate CRC */
  return CRC_Calculate((const uint8_t *)data, size * sizeof(uint32_t));
}

/**
 * @brief Get last CRC result
 * @retval Last CRC result
 */
uint32_t CRC_GetResult(void)
{
  return g_crc_result;
}

/**
 * @brief Reset CRC peripheral (stub)
 */
void CRC_Reset(void)
{
  g_crc_result = 0xFFFFFFFFUL;
}
