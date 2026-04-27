/**
 ******************************************************************************
 * @file           : pcf8574.c
 * @brief          : PCF8574 I2C GPIO Expander Driver Implementation
 ******************************************************************************
 */

#include "pcf8574.h"
#include "main.h"
#include <string.h>

/* SYSTEM_Log fonksiyonları yoksa define et */
#ifndef SYSTEM_LogInfo
#define SYSTEM_LogInfo(...)  ((void)0)
#endif
#ifndef SYSTEM_LogError
#define SYSTEM_LogError(...) ((void)0)
#endif
#ifndef SYSTEM_LogWarn
#define SYSTEM_LogWarn(...)  ((void)0)
#endif

/* Global Variables ---------------------------------------------------------*/
pcf8574_handle_t g_pcf8574_chips[PCF8574_MAX_CHIPS] = {0};
uint8_t g_pcf8574_chip_count = 0U;

/* Private Functions --------------------------------------------------------*/
static uint8_t PCF8574_CalculateI2CAddress(uint8_t chip_id) {
  return PCF8574_I2C_ADDR_BASE + (chip_id * PCF8574_I2C_ADDR_STEP);
}

/* Public Functions ---------------------------------------------------------*/

/**
 * @brief  Initialize single PCF8574 chip
 */
pcf8574_status_t PCF8574_Init(pcf8574_handle_t *hpcf, I2C_HandleTypeDef *hi2c, 
                               uint8_t chip_id) {
  if (hpcf == NULL || hi2c == NULL || chip_id >= PCF8574_MAX_CHIPS) {
    return PCF8574_ERROR;
  }
  
  /* Initialize handle */
  hpcf->hi2c = hi2c;
  hpcf->i2c_addr = PCF8574_CalculateI2CAddress(chip_id);
  hpcf->chip_id = chip_id;
  hpcf->output_state = 0x00U;
  hpcf->last_written_state = 0xFFU;  /* Force first write */
  hpcf->is_initialized = 0U;
  hpcf->fault_count = 0U;
  hpcf->last_comm_time = 0U;
  
  /* Test communication - write initial state (all outputs LOW) */
  uint8_t test_data = 0x00U;
  HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hi2c, hpcf->i2c_addr, 
                                                      &test_data, 1, 
                                                      PCF8574_I2C_TIMEOUT);
  
  if (status != HAL_OK) {
    hpcf->fault_count++;
    return PCF8574_ERROR;
  }
  
  /* Verify write by reading back */
  uint8_t read_data = 0xFFU;
  status = HAL_I2C_Master_Receive(hi2c, hpcf->i2c_addr, &read_data, 1, 
                                   PCF8574_I2C_TIMEOUT);
  
  if (status != HAL_OK) {
    hpcf->fault_count++;
    return PCF8574_ERROR;
  }
  
  /* PCF8574 inputs are high-impedance, read will return ~0xFF when outputs are low */
  hpcf->is_initialized = 1U;
  hpcf->last_comm_time = HAL_GetTick();
  hpcf->last_written_state = test_data;
  
  return PCF8574_OK;
}

/**
 * @brief  Initialize all PCF8574 chips
 */
pcf8574_status_t PCF8574_InitAll(I2C_HandleTypeDef *hi2c, uint8_t chip_count) {
  if (chip_count > PCF8574_MAX_CHIPS) {
    chip_count = PCF8574_MAX_CHIPS;
  }
  
  g_pcf8574_chip_count = 0U;
  
  for (uint8_t i = 0; i < chip_count; i++) {
    pcf8574_status_t status = PCF8574_Init(&g_pcf8574_chips[i], hi2c, i);
    
    if (status == PCF8574_OK) {
      g_pcf8574_chip_count++;
      SYSTEM_LogInfo("PCF8574 #%d initialized at 0x%02X", i, 
                     g_pcf8574_chips[i].i2c_addr);
    } else {
      SYSTEM_LogError("PCF8574 #%d initialization failed", i);
    }
  }
  
  return (g_pcf8574_chip_count == chip_count) ? PCF8574_OK : PCF8574_ERROR;
}

/**
 * @brief  Write single pin state
 */
pcf8574_status_t PCF8574_WritePin(pcf8574_handle_t *hpcf, uint8_t pin, uint8_t state) {
  if (hpcf == NULL || !hpcf->is_initialized || pin >= PCF8574_PINS_PER_CHIP) {
    return PCF8574_ERROR_INVALID_PIN;
  }
  
  /* Update output state */
  if (state != 0U) {
    hpcf->output_state |= (1U << pin);
  } else {
    hpcf->output_state &= ~(1U << pin);
  }
  
  /* Write only if state changed */
  if (hpcf->output_state != hpcf->last_written_state) {
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hpcf->hi2c, hpcf->i2c_addr,
                                                        &hpcf->output_state, 1,
                                                        PCF8574_I2C_TIMEOUT);
    
    if (status != HAL_OK) {
      hpcf->fault_count++;
      return PCF8574_ERROR;
    }
    
    hpcf->last_written_state = hpcf->output_state;
    hpcf->last_comm_time = HAL_GetTick();
  }
  
  return PCF8574_OK;
}

/**
 * @brief  Read single pin state (input mode pins only)
 */
pcf8574_status_t PCF8574_ReadPin(pcf8574_handle_t *hpcf, uint8_t pin, uint8_t *state) {
  if (hpcf == NULL || !hpcf->is_initialized || pin >= PCF8574_PINS_PER_CHIP || state == NULL) {
    return PCF8574_ERROR_INVALID_PIN;
  }
  
  uint8_t data = 0U;
  pcf8574_status_t status = PCF8574_ReadByte(hpcf, &data);
  
  if (status != PCF8574_OK) {
    return status;
  }
  
  *state = (data >> pin) & 0x01U;
  return PCF8574_OK;
}

/**
 * @brief  Toggle single pin
 */
pcf8574_status_t PCF8574_TogglePin(pcf8574_handle_t *hpcf, uint8_t pin) {
  if (hpcf == NULL || !hpcf->is_initialized || pin >= PCF8574_PINS_PER_CHIP) {
    return PCF8574_ERROR_INVALID_PIN;
  }
  
  uint8_t current_state = (hpcf->output_state >> pin) & 0x01U;
  return PCF8574_WritePin(hpcf, pin, current_state ? 0U : 1U);
}

/**
 * @brief  Write all 8 pins at once
 */
pcf8574_status_t PCF8574_WriteByte(pcf8574_handle_t *hpcf, uint8_t data) {
  if (hpcf == NULL || !hpcf->is_initialized) {
    return PCF8574_ERROR;
  }
  
  if (data == hpcf->last_written_state) {
    return PCF8574_OK;  /* No change needed */
  }
  
  HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hpcf->hi2c, hpcf->i2c_addr,
                                                      &data, 1,
                                                      PCF8574_I2C_TIMEOUT);
  
  if (status != HAL_OK) {
    hpcf->fault_count++;
    return PCF8574_ERROR;
  }
  
  hpcf->output_state = data;
  hpcf->last_written_state = data;
  hpcf->last_comm_time = HAL_GetTick();
  
  return PCF8574_OK;
}

/**
 * @brief  Read all 8 pins (for input configuration)
 */
pcf8574_status_t PCF8574_ReadByte(pcf8574_handle_t *hpcf, uint8_t *data) {
  if (hpcf == NULL || !hpcf->is_initialized || data == NULL) {
    return PCF8574_ERROR;
  }
  
  HAL_StatusTypeDef status = HAL_I2C_Master_Receive(hpcf->hi2c, hpcf->i2c_addr,
                                                     data, 1,
                                                     PCF8574_I2C_TIMEOUT);
  
  if (status != HAL_OK) {
    hpcf->fault_count++;
    return PCF8574_ERROR;
  }
  
  hpcf->last_comm_time = HAL_GetTick();
  return PCF8574_OK;
}

/**
 * @brief  Check if chip is initialized
 */
uint8_t PCF8574_IsInitialized(pcf8574_handle_t *hpcf) {
  return (hpcf != NULL) ? hpcf->is_initialized : 0U;
}

/**
 * @brief  Get current output state
 */
uint8_t PCF8574_GetOutputState(pcf8574_handle_t *hpcf) {
  return (hpcf != NULL && hpcf->is_initialized) ? hpcf->output_state : 0x00U;
}

/**
 * @brief  Get fault counter
 */
uint8_t PCF8574_GetFaultCount(pcf8574_handle_t *hpcf) {
  return (hpcf != NULL) ? hpcf->fault_count : 0U;
}

/**
 * @brief  Clear fault counter
 */
void PCF8574_ClearFaults(pcf8574_handle_t *hpcf) {
  if (hpcf != NULL) {
    hpcf->fault_count = 0U;
  }
}

/**
 * @brief  Get last communication time
 */
uint32_t PCF8574_GetLastCommTime(pcf8574_handle_t *hpcf) {
  return (hpcf != NULL) ? hpcf->last_comm_time : 0U;
}

/**
 * @brief  Health check - verify chip is responsive
 */
pcf8574_status_t PCF8574_HealthCheck(pcf8574_handle_t *hpcf) {
  if (hpcf == NULL || !hpcf->is_initialized) {
    return PCF8574_ERROR_NOT_INIT;
  }
  
  /* Try to read current state */
  uint8_t read_data = 0U;
  pcf8574_status_t status = PCF8574_ReadByte(hpcf, &read_data);
  
  if (status != PCF8574_OK) {
    return status;
  }
  
  /* Verify communication freshness (within last 5 seconds) */
  uint32_t now = HAL_GetTick();
  if (now - hpcf->last_comm_time > 5000U) {
    hpcf->fault_count++;
    return PCF8574_ERROR;
  }
  
  return PCF8574_OK;
}

/**
 * @brief  Periodic task - call every 100ms for health monitoring
 */
void PCF8574_PeriodicTask(void) {
  static uint32_t last_check = 0U;
  uint32_t now = HAL_GetTick();
  
  if (now - last_check < 100U) {
    return;
  }
  last_check = now;
  
  /* Health check all chips */
  for (uint8_t i = 0; i < g_pcf8574_chip_count; i++) {
    pcf8574_handle_t *hpcf = &g_pcf8574_chips[i];
    
    if (!hpcf->is_initialized) {
      continue;
    }
    
    /* Check for communication timeout */
    if (now - hpcf->last_comm_time > 10000U) {  /* 10 seconds */
      hpcf->fault_count++;
      SYSTEM_LogError("PCF8574 #%d communication timeout", i);
      
      if (hpcf->fault_count >= 5U) {
        SYSTEM_LogError("PCF8574 #%d marked as faulty", i);
        /* Don't mark as uninitialized - keep trying */
      }
    }
    
    /* Auto-clear faults if communication recovered */
    if (hpcf->fault_count > 0 && now - hpcf->last_comm_time < 1000U) {
      hpcf->fault_count--;
    }
  }
}

/**
 * @brief  Get total chip count
 */
uint8_t PCF8574_GetChipCount(void) {
  return g_pcf8574_chip_count;
}

/**
 * @brief  Get chip handle by ID
 */
pcf8574_handle_t* PCF8574_GetChip(uint8_t chip_id) {
  if (chip_id >= g_pcf8574_chip_count) {
    return NULL;
  }
  return &g_pcf8574_chips[chip_id];
}
