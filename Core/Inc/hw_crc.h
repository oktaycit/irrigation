/**
 ******************************************************************************
 * @file           : hw_crc.h
 * @brief          : Hardware CRC Driver Header (STM32F4 CRC peripheral)
 ******************************************************************************
 */

#ifndef __HW_CRC_H
#define __HW_CRC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

/* CRC Configuration ---------------------------------------------------------*/
/* STM32F4 CRC peripheral uses CRC-32 (Ethernet) polynomial: 0x4C11DB7 */
/* Polynomial: X^32 + X^26 + X^23 + X^22 + X^16 + X^12 + X^11 + X^10 + X^8 + X^7 + X^5 + X^4 + X^2 + X + 1 */

/* CRC Functions -------------------------------------------------------------*/
uint8_t CRC_Init(void);
uint32_t CRC_Calculate(const uint8_t *data, size_t size);
uint32_t CRC_Calculate32(const uint32_t *data, size_t size);
uint32_t CRC_GetResult(void);
void CRC_Reset(void);

/* Compatibility macros for software CRC replacement */
#define CRC16_Init() CRC_Init()
#define CRC16_Calculate(data, size) ((uint16_t)(CRC_Calculate(data, size) & 0xFFFF))

#ifdef __cplusplus
}
#endif

#endif /* __HW_CRC_H */
