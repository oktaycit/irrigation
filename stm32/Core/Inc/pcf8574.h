/**
 ******************************************************************************
 * @file           : pcf8574.h
 * @brief          : PCF8574 I2C GPIO Expander Driver
 ******************************************************************************
 */

#ifndef __PCF8574_H
#define __PCF8574_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>

/* PCF8574 Configuration ----------------------------------------------------*/
#define PCF8574_I2C_ADDR_BASE    0x20U  /* A2=0, A1=0, A0=0 */
#define PCF8574_I2C_ADDR_STEP    0x01U  /* Address step for each chip */
#define PCF8574_MAX_CHIPS        8U     /* Maximum 8 chips on I2C bus */
#define PCF8574_PINS_PER_CHIP    8U     /* 8 GPIO pins per chip */
#define PCF8574_I2C_TIMEOUT      100U   /* I2C timeout in ms */

/* PCF8574 Handle Structure -------------------------------------------------*/
typedef struct {
  I2C_HandleTypeDef *hi2c;      /* I2C handle */
  uint8_t i2c_addr;              /* I2C address (0x20-0x27) */
  uint8_t chip_id;               /* Chip ID (0-7) */
  uint8_t output_state;          /* Current output state (P0-P7) */
  uint8_t last_written_state;    /* Last successfully written state */
  uint8_t is_initialized;        /* Initialization flag */
  uint8_t fault_count;           /* Communication fault counter */
  uint32_t last_comm_time;       /* Last successful communication time */
} pcf8574_handle_t;

/* PCF8574 Status -----------------------------------------------------------*/
typedef enum {
  PCF8574_OK = 0,
  PCF8574_ERROR,
  PCF8574_ERROR_TIMEOUT,
  PCF8574_ERROR_NOT_INIT,
  PCF8574_ERROR_INVALID_PIN
} pcf8574_status_t;

/* Global PCF8574 Handles ---------------------------------------------------*/
extern pcf8574_handle_t g_pcf8574_chips[PCF8574_MAX_CHIPS];
extern uint8_t g_pcf8574_chip_count;

/* Public Functions ---------------------------------------------------------*/

/* Initialization */
pcf8574_status_t PCF8574_Init(pcf8574_handle_t *hpcf, I2C_HandleTypeDef *hi2c, 
                               uint8_t chip_id);
pcf8574_status_t PCF8574_InitAll(I2C_HandleTypeDef *hi2c, uint8_t chip_count);

/* Pin Control */
pcf8574_status_t PCF8574_WritePin(pcf8574_handle_t *hpcf, uint8_t pin, uint8_t state);
pcf8574_status_t PCF8574_ReadPin(pcf8574_handle_t *hpcf, uint8_t pin, uint8_t *state);
pcf8574_status_t PCF8574_TogglePin(pcf8574_handle_t *hpcf, uint8_t pin);

/* Byte Control (all 8 pins at once) */
pcf8574_status_t PCF8574_WriteByte(pcf8574_handle_t *hpcf, uint8_t data);
pcf8574_status_t PCF8574_ReadByte(pcf8574_handle_t *hpcf, uint8_t *data);

/* Status & Diagnostics */
uint8_t PCF8574_IsInitialized(pcf8574_handle_t *hpcf);
uint8_t PCF8574_GetOutputState(pcf8574_handle_t *hpcf);
uint8_t PCF8574_GetFaultCount(pcf8574_handle_t *hpcf);
void PCF8574_ClearFaults(pcf8574_handle_t *hpcf);
uint32_t PCF8574_GetLastCommTime(pcf8574_handle_t *hpcf);

/* Health Check */
pcf8574_status_t PCF8574_HealthCheck(pcf8574_handle_t *hpcf);
void PCF8574_PeriodicTask(void);  /* Call every 100ms */

/* Utility */
uint8_t PCF8574_GetChipCount(void);
pcf8574_handle_t* PCF8574_GetChip(uint8_t chip_id);

#ifdef __cplusplus
}
#endif

#endif /* __PCF8574_H */
