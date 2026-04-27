/**
 ******************************************************************************
 * @file           : hw_crc_stub.c
 * @brief          : Hardware CRC Driver using STM32F4 CRC peripheral
 ******************************************************************************
 * @note           : Uses direct registers because the HAL CRC driver is not
 *                   bundled in this project.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "hw_crc.h"

/* Private variables ---------------------------------------------------------*/
static uint32_t g_crc_result = 0U;
static uint8_t g_crc_initialized = 0;

/* Private functions ---------------------------------------------------------*/
static uint32_t CRC_PackPartialWord(const uint8_t *data, size_t size);

/**
 * @brief Initialize hardware CRC peripheral
 * @retval 1 if success
 */
uint8_t CRC_Init(void)
{
  __HAL_RCC_CRC_CLK_ENABLE();
  CRC_Reset();
  g_crc_initialized = 1;
  return 1;
}

/**
 * @brief Calculate STM32 hardware CRC-32
 * @param data Data buffer
 * @param size Buffer size
 * @retval CRC-32 value
 */
uint32_t CRC_Calculate(const uint8_t *data, size_t size)
{
  size_t word_count;
  size_t remaining_bytes;
  
  if (g_crc_initialized == 0U || data == NULL || size == 0U) {
    return 0;
  }

  CRC_Reset();

  word_count = size / sizeof(uint32_t);
  remaining_bytes = size % sizeof(uint32_t);

  for (size_t i = 0U; i < word_count; i++) {
    CRC->DR = CRC_PackPartialWord(&data[i * sizeof(uint32_t)],
                                  sizeof(uint32_t));
  }

  if (remaining_bytes != 0U) {
    CRC->DR = CRC_PackPartialWord(&data[word_count * sizeof(uint32_t)],
                                  remaining_bytes);
  }
  
  g_crc_result = CRC->DR;
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
  if (g_crc_initialized == 0U || data == NULL || size == 0U) {
    return 0;
  }

  CRC_Reset();

  for (size_t i = 0U; i < size; i++) {
    CRC->DR = data[i];
  }

  g_crc_result = CRC->DR;
  return g_crc_result;
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
  __HAL_RCC_CRC_CLK_ENABLE();
  CRC->CR = CRC_CR_RESET;
  g_crc_result = CRC->DR;
}

static uint32_t CRC_PackPartialWord(const uint8_t *data, size_t size)
{
  uint32_t word = 0U;

  for (size_t i = 0U; i < size; i++) {
    word |= ((uint32_t)data[i] << (8U * i));
  }

  return word;
}
