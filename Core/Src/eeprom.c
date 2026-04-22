/**
 ******************************************************************************
 * @file           : eeprom.c
 * @brief          : Persistent storage implementation on W25Q16 SPI flash
 ******************************************************************************
 */

#include "main.h"
#include <string.h>

extern SPI_HandleTypeDef hspi1;

static eeprom_handle_t h_eeprom = {0};
static uint8_t g_eeprom_sector_cache[4096] = {0};

static uint8_t EEPROM_IsStructCRCValid(void *data, uint16_t size,
                                       uint16_t expected_crc);
static void EEPROM_SetDefaultSystemParams(void);
static void EEPROM_SetDefaultPrograms(void);
static uint16_t EEPROM_GetProgramAddress(uint8_t program_id);
static void EEPROM_FlashSelect(void);
static void EEPROM_FlashDeselect(void);
static uint8_t EEPROM_FlashWaitReady(uint32_t timeout_ms);
static uint8_t EEPROM_FlashWriteEnable(void);
static uint8_t EEPROM_FlashRead(uint32_t address, uint8_t *buffer, uint32_t size);
static uint8_t EEPROM_FlashPageProgram(uint32_t address, const uint8_t *buffer,
                                       uint16_t size);
static uint8_t EEPROM_FlashEraseSector(uint32_t address);
static uint8_t EEPROM_FlashInit(void);

#define EEPROM_FLASH_CS_PORT GPIOB
#define EEPROM_FLASH_CS_PIN GPIO_PIN_0
#define EEPROM_FLASH_CMD_JEDEC_ID 0x9FU
#define EEPROM_FLASH_CMD_READ_DATA 0x03U
#define EEPROM_FLASH_CMD_PAGE_PROGRAM 0x02U
#define EEPROM_FLASH_CMD_WRITE_ENABLE 0x06U
#define EEPROM_FLASH_CMD_READ_STATUS1 0x05U
#define EEPROM_FLASH_CMD_SECTOR_ERASE 0x20U
#define EEPROM_FLASH_STATUS_BUSY 0x01U
#define EEPROM_FLASH_SECTOR_SIZE 4096U
#define EEPROM_FLASH_PAGE_SIZE 256U
#define EEPROM_FLASH_EXPECTED_MFG 0xEFU
#define EEPROM_FLASH_EXPECTED_CAPACITY 0x15U

uint8_t EEPROM_Init(void) {
  if (EEPROM_FlashInit() != EEPROM_OK) {
    h_eeprom.last_error = EEPROM_NOT_INITIALIZED;
    return EEPROM_ERROR;
  }

  if (EEPROM_ReadBuffer(EEPROM_ADDR_MAGIC, (uint8_t *)&h_eeprom.header,
                        sizeof(h_eeprom.header)) != EEPROM_OK) {
    return EEPROM_ERROR;
  }

  if (h_eeprom.header.magic != EEPROM_MAGIC_VALUE) {
    EEPROM_Format();
  }

  h_eeprom.initialized = 1U;
  (void)EEPROM_LoadSystemParams();
  return EEPROM_OK;
}

uint8_t EEPROM_ReadBuffer(uint16_t address, uint8_t *buffer, uint16_t size) {
  if (buffer == NULL || size == 0U) {
    h_eeprom.last_error = EEPROM_READ_ERROR;
    return EEPROM_ERROR;
  }

  if (EEPROM_FlashRead(address, buffer, size) != EEPROM_OK) {
    h_eeprom.last_error = EEPROM_READ_ERROR;
    return EEPROM_ERROR;
  }

  return EEPROM_OK;
}

uint8_t EEPROM_WriteBuffer(uint16_t address, uint8_t *buffer, uint16_t size) {
  uint32_t write_start = address;
  uint32_t write_end = (uint32_t)address + size;
  uint32_t sector_base;

  if (buffer == NULL || size == 0U) {
    h_eeprom.last_error = EEPROM_WRITE_ERROR;
    return EEPROM_ERROR;
  }

  for (sector_base = (write_start / EEPROM_FLASH_SECTOR_SIZE) * EEPROM_FLASH_SECTOR_SIZE;
       sector_base < write_end;
       sector_base += EEPROM_FLASH_SECTOR_SIZE) {
    uint32_t sector_offset = (write_start > sector_base) ? (write_start - sector_base) : 0U;
    uint32_t copy_start = sector_base + sector_offset;
    uint32_t copy_end = write_end;
    uint32_t copy_length;
    uint32_t page_address;

    if (copy_end > (sector_base + EEPROM_FLASH_SECTOR_SIZE)) {
      copy_end = sector_base + EEPROM_FLASH_SECTOR_SIZE;
    }
    copy_length = copy_end - copy_start;

    if (EEPROM_FlashRead(sector_base, g_eeprom_sector_cache, EEPROM_FLASH_SECTOR_SIZE) !=
        EEPROM_OK) {
      h_eeprom.last_error = EEPROM_READ_ERROR;
      return EEPROM_ERROR;
    }

    memcpy(&g_eeprom_sector_cache[sector_offset],
           &buffer[copy_start - write_start],
           copy_length);

    if (EEPROM_FlashEraseSector(sector_base) != EEPROM_OK) {
      h_eeprom.last_error = EEPROM_WRITE_ERROR;
      return EEPROM_ERROR;
    }

    for (page_address = 0U; page_address < EEPROM_FLASH_SECTOR_SIZE;
         page_address += EEPROM_FLASH_PAGE_SIZE) {
      if (EEPROM_FlashPageProgram(sector_base + page_address,
                                  &g_eeprom_sector_cache[page_address],
                                  EEPROM_FLASH_PAGE_SIZE) != EEPROM_OK) {
        h_eeprom.last_error = EEPROM_WRITE_ERROR;
        return EEPROM_ERROR;
      }
    }
  }

  return EEPROM_OK;
}

uint8_t EEPROM_LoadSystemParams(void) {
  if (EEPROM_ReadBuffer(EEPROM_ADDR_SYSTEM, (uint8_t *)&h_eeprom.system,
                        sizeof(h_eeprom.system)) != EEPROM_OK) {
    return EEPROM_ERROR;
  }

  if (!EEPROM_IsStructCRCValid(&h_eeprom.system, sizeof(h_eeprom.system),
                               h_eeprom.system.crc)) {
    EEPROM_SetDefaultSystemParams();
    (void)EEPROM_SaveSystemParams();
    h_eeprom.last_error = EEPROM_CRC_ERROR;
    return EEPROM_ERROR;
  }

  return EEPROM_OK;
}

uint8_t EEPROM_SaveSystemParams(void) {
  h_eeprom.system.crc =
      EEPROM_CalculateCRC(&h_eeprom.system, sizeof(h_eeprom.system) - 2U);
  return EEPROM_WriteBuffer(EEPROM_ADDR_SYSTEM, (uint8_t *)&h_eeprom.system,
                            sizeof(h_eeprom.system));
}

uint8_t EEPROM_ResetSystemParams(void) {
  EEPROM_SetDefaultSystemParams();
  return EEPROM_SaveSystemParams();
}

uint8_t EEPROM_GetSystemParams(eeprom_system_t *system) {
  if (system == NULL) {
    return EEPROM_ERROR;
  }

  *system = h_eeprom.system;
  return EEPROM_OK;
}

uint8_t EEPROM_SetSystemParams(const eeprom_system_t *system) {
  if (system == NULL) {
    return EEPROM_ERROR;
  }

  h_eeprom.system = *system;
  return EEPROM_SaveSystemParams();
}

uint8_t EEPROM_LoadTouchCalibration(touch_calibration_t *cal) {
  eeprom_touch_cal_t record = {0};

  if (cal == NULL) {
    h_eeprom.last_error = EEPROM_READ_ERROR;
    return EEPROM_ERROR;
  }

  if (EEPROM_ReadBuffer(EEPROM_ADDR_TOUCH_CAL, (uint8_t *)&record,
                        sizeof(record)) != EEPROM_OK) {
    return EEPROM_ERROR;
  }

  if ((record.calibration.valid == 0U) ||
      !EEPROM_IsStructCRCValid(&record.calibration, sizeof(record.calibration),
                               record.crc)) {
    memset(cal, 0, sizeof(*cal));
    h_eeprom.last_error = EEPROM_CRC_ERROR;
    return EEPROM_ERROR;
  }

  *cal = record.calibration;
  return EEPROM_OK;
}

uint8_t EEPROM_SaveTouchCalibration(const touch_calibration_t *cal) {
  eeprom_touch_cal_t record = {0};

  if (cal == NULL) {
    h_eeprom.last_error = EEPROM_WRITE_ERROR;
    return EEPROM_ERROR;
  }

  record.calibration = *cal;
  record.crc = EEPROM_CalculateCRC(&record.calibration, sizeof(record.calibration));
  return EEPROM_WriteBuffer(EEPROM_ADDR_TOUCH_CAL, (uint8_t *)&record,
                            sizeof(record));
}

uint8_t EEPROM_LoadProgram(uint8_t program_id, irrigation_program_t *program) {
  irrigation_program_t record = {0};

  if (program_id == 0U || program_id > VALVE_COUNT || program == NULL) {
    h_eeprom.last_error = EEPROM_READ_ERROR;
    return EEPROM_ERROR;
  }

  if (EEPROM_ReadBuffer(EEPROM_GetProgramAddress(program_id), (uint8_t *)&record,
                        sizeof(record)) != EEPROM_OK) {
    return EEPROM_ERROR;
  }

  if (!EEPROM_IsStructCRCValid(&record, sizeof(record), record.crc)) {
    memset(program, 0, sizeof(*program));
    return EEPROM_ERROR;
  }

  *program = record;
  return EEPROM_OK;
}

uint8_t EEPROM_SaveProgram(uint8_t program_id,
                           const irrigation_program_t *program) {
  irrigation_program_t record = {0};

  if (program_id == 0U || program_id > VALVE_COUNT || program == NULL) {
    h_eeprom.last_error = EEPROM_WRITE_ERROR;
    return EEPROM_ERROR;
  }

  record = *program;
  record.crc = EEPROM_CalculateCRC(&record, sizeof(record) - 2U);
  return EEPROM_WriteBuffer(EEPROM_GetProgramAddress(program_id),
                            (uint8_t *)&record, sizeof(record));
}

uint8_t EEPROM_LoadAllPrograms(irrigation_program_t *programs, uint8_t count) {
  uint8_t max_count = (count > VALVE_COUNT) ? VALVE_COUNT : count;

  if (programs == NULL) {
    return EEPROM_ERROR;
  }

  for (uint8_t i = 0U; i < max_count; i++) {
    if (EEPROM_LoadProgram(i + 1U, &programs[i]) != EEPROM_OK) {
      memset(&programs[i], 0, sizeof(programs[i]));
    }
  }

  return EEPROM_OK;
}

uint8_t EEPROM_SaveRuntimeBackup(
    const irrigation_runtime_backup_t *runtime_backup) {
  irrigation_runtime_backup_t record = {0};

  if (runtime_backup == NULL) {
    return EEPROM_ERROR;
  }

  record = *runtime_backup;
  record.crc = EEPROM_CalculateCRC(&record, sizeof(record) - 2U);
  return EEPROM_WriteBuffer(EEPROM_ADDR_RUNTIME_BACKUP, (uint8_t *)&record,
                            sizeof(record));
}

uint8_t EEPROM_LoadRuntimeBackup(irrigation_runtime_backup_t *runtime_backup) {
  irrigation_runtime_backup_t record = {0};

  if (runtime_backup == NULL) {
    return EEPROM_ERROR;
  }

  if (EEPROM_ReadBuffer(EEPROM_ADDR_RUNTIME_BACKUP, (uint8_t *)&record,
                        sizeof(record)) != EEPROM_OK) {
    return EEPROM_ERROR;
  }

  if (record.valid == 0U ||
      !EEPROM_IsStructCRCValid(&record, sizeof(record), record.crc)) {
    memset(runtime_backup, 0, sizeof(*runtime_backup));
    h_eeprom.last_error = EEPROM_CRC_ERROR;
    return EEPROM_ERROR;
  }

  *runtime_backup = record;
  return EEPROM_OK;
}

uint8_t EEPROM_ClearRuntimeBackup(void) {
  irrigation_runtime_backup_t record = {0};
  return EEPROM_WriteBuffer(EEPROM_ADDR_RUNTIME_BACKUP, (uint8_t *)&record,
                            sizeof(record));
}

void EEPROM_Format(void) {
  memset(&h_eeprom, 0, sizeof(h_eeprom));

  h_eeprom.header.magic = EEPROM_MAGIC_VALUE;
  h_eeprom.header.version_major = FIRMWARE_VERSION_MAJOR;
  h_eeprom.header.version_minor = (uint8_t)((FIRMWARE_VERSION_MINOR << 4U) |
                                            (FIRMWARE_VERSION_PATCH & 0x0FU));
  h_eeprom.header.initialized = 1U;

  (void)EEPROM_WriteBuffer(EEPROM_ADDR_MAGIC, (uint8_t *)&h_eeprom.header,
                           sizeof(h_eeprom.header));

  EEPROM_SetDefaultSystemParams();
  (void)EEPROM_SaveSystemParams();
  EEPROM_SetDefaultPrograms();
  (void)EEPROM_ClearRuntimeBackup();
}

uint16_t EEPROM_CalculateCRC(void *data, uint16_t size) {
  uint16_t crc = 0xFFFFU;
  uint8_t *bytes = (uint8_t *)data;

  if (data == NULL || size == 0U) {
    return 0U;
  }

  for (uint16_t i = 0U; i < size; i++) {
    crc ^= bytes[i];
    for (uint8_t bit = 0U; bit < 8U; bit++) {
      crc = (crc & 1U) != 0U ? (uint16_t)((crc >> 1U) ^ 0xA001U)
                             : (uint16_t)(crc >> 1U);
    }
  }

  return crc;
}

uint8_t EEPROM_GetLastError(void) { return h_eeprom.last_error; }

static uint8_t EEPROM_IsStructCRCValid(void *data, uint16_t size,
                                       uint16_t expected_crc) {
  if (size < 2U) {
    return 0U;
  }

  return (EEPROM_CalculateCRC(data, size - 2U) == expected_crc) ? 1U : 0U;
}

static void EEPROM_SetDefaultSystemParams(void) {
  memset(&h_eeprom.system, 0, sizeof(h_eeprom.system));
  h_eeprom.system.brightness = 80U;
  h_eeprom.system.ph_target = 6.50f;
  h_eeprom.system.ph_min = 5.50f;
  h_eeprom.system.ph_max = 7.50f;
  h_eeprom.system.ec_target = 1.80f;
  h_eeprom.system.ec_min = 1.00f;
  h_eeprom.system.ec_max = 2.50f;
  h_eeprom.system.irrigation_interval = 60U;
  h_eeprom.system.auto_mode_enabled = 1U;
  h_eeprom.system.reserved[0] = AUTO_MODE_SCHEDULED;
  h_eeprom.system.reserved[1] = EEPROM_SYSTEM_MODE_MARKER;
}

static void EEPROM_SetDefaultPrograms(void) {
  irrigation_program_t program = {0};

  for (uint8_t i = 0U; i < VALVE_COUNT; i++) {
    memset(&program, 0, sizeof(program));
    program.enabled = 0U;
    program.start_hhmm = (uint16_t)(600U + (i * 10U));
    program.end_hhmm = (uint16_t)(program.start_hhmm + 100U);
    program.valve_mask = (uint8_t)(1U << i);
    program.irrigation_min = 5U;
    program.wait_min = 1U;
    program.repeat_count = 1U;
    program.days_mask = 0x7FU;
    program.ec_set_x100 = 180;
    program.ph_set_x100 = 650;
    program.last_run_day = 0U;
    program.last_run_minute = 0xFFFFU;
    (void)EEPROM_SaveProgram(i + 1U, &program);
  }
}

static uint16_t EEPROM_GetProgramAddress(uint8_t program_id) {
  return (uint16_t)(EEPROM_ADDR_PROGRAMS +
                    ((uint16_t)(program_id - 1U) * sizeof(irrigation_program_t)));
}

static void EEPROM_FlashSelect(void) {
  HAL_GPIO_WritePin(EEPROM_FLASH_CS_PORT, EEPROM_FLASH_CS_PIN, GPIO_PIN_RESET);
}

static void EEPROM_FlashDeselect(void) {
  HAL_GPIO_WritePin(EEPROM_FLASH_CS_PORT, EEPROM_FLASH_CS_PIN, GPIO_PIN_SET);
}

static uint8_t EEPROM_FlashWaitReady(uint32_t timeout_ms) {
  uint8_t command = EEPROM_FLASH_CMD_READ_STATUS1;
  uint8_t status = EEPROM_FLASH_STATUS_BUSY;
  uint32_t start = HAL_GetTick();

  while ((status & EEPROM_FLASH_STATUS_BUSY) != 0U) {
    EEPROM_FlashSelect();
    if (HAL_SPI_Transmit(&hspi1, &command, 1U, 100U) != HAL_OK ||
        HAL_SPI_Receive(&hspi1, &status, 1U, 100U) != HAL_OK) {
      EEPROM_FlashDeselect();
      return EEPROM_ERROR;
    }
    EEPROM_FlashDeselect();

    if ((HAL_GetTick() - start) > timeout_ms) {
      return EEPROM_ERROR;
    }
  }

  return EEPROM_OK;
}

static uint8_t EEPROM_FlashWriteEnable(void) {
  uint8_t command = EEPROM_FLASH_CMD_WRITE_ENABLE;

  EEPROM_FlashSelect();
  if (HAL_SPI_Transmit(&hspi1, &command, 1U, 100U) != HAL_OK) {
    EEPROM_FlashDeselect();
    return EEPROM_ERROR;
  }
  EEPROM_FlashDeselect();
  return EEPROM_OK;
}

static uint8_t EEPROM_FlashRead(uint32_t address, uint8_t *buffer, uint32_t size) {
  uint8_t command[4];

  if (buffer == NULL || size == 0U) {
    return EEPROM_ERROR;
  }

  command[0] = EEPROM_FLASH_CMD_READ_DATA;
  command[1] = (uint8_t)(address >> 16U);
  command[2] = (uint8_t)(address >> 8U);
  command[3] = (uint8_t)address;

  EEPROM_FlashSelect();
  if (HAL_SPI_Transmit(&hspi1, command, sizeof(command), 100U) != HAL_OK ||
      HAL_SPI_Receive(&hspi1, buffer, size, 1000U) != HAL_OK) {
    EEPROM_FlashDeselect();
    return EEPROM_ERROR;
  }
  EEPROM_FlashDeselect();
  return EEPROM_OK;
}

static uint8_t EEPROM_FlashPageProgram(uint32_t address, const uint8_t *buffer,
                                       uint16_t size) {
  uint8_t command[4];

  if (buffer == NULL || size == 0U || size > EEPROM_FLASH_PAGE_SIZE) {
    return EEPROM_ERROR;
  }

  if (EEPROM_FlashWriteEnable() != EEPROM_OK) {
    return EEPROM_ERROR;
  }

  command[0] = EEPROM_FLASH_CMD_PAGE_PROGRAM;
  command[1] = (uint8_t)(address >> 16U);
  command[2] = (uint8_t)(address >> 8U);
  command[3] = (uint8_t)address;

  EEPROM_FlashSelect();
  if (HAL_SPI_Transmit(&hspi1, command, sizeof(command), 100U) != HAL_OK ||
      HAL_SPI_Transmit(&hspi1, (uint8_t *)buffer, size, 1000U) != HAL_OK) {
    EEPROM_FlashDeselect();
    return EEPROM_ERROR;
  }
  EEPROM_FlashDeselect();

  return EEPROM_FlashWaitReady(1000U);
}

static uint8_t EEPROM_FlashEraseSector(uint32_t address) {
  uint8_t command[4];

  if (EEPROM_FlashWriteEnable() != EEPROM_OK) {
    return EEPROM_ERROR;
  }

  command[0] = EEPROM_FLASH_CMD_SECTOR_ERASE;
  command[1] = (uint8_t)(address >> 16U);
  command[2] = (uint8_t)(address >> 8U);
  command[3] = (uint8_t)address;

  EEPROM_FlashSelect();
  if (HAL_SPI_Transmit(&hspi1, command, sizeof(command), 100U) != HAL_OK) {
    EEPROM_FlashDeselect();
    return EEPROM_ERROR;
  }
  EEPROM_FlashDeselect();

  return EEPROM_FlashWaitReady(3000U);
}

static uint8_t EEPROM_FlashInit(void) {
  uint8_t command = EEPROM_FLASH_CMD_JEDEC_ID;
  uint8_t id[3] = {0};

  EEPROM_FlashDeselect();
  if (EEPROM_FlashWaitReady(100U) != EEPROM_OK) {
    return EEPROM_ERROR;
  }

  EEPROM_FlashSelect();
  if (HAL_SPI_Transmit(&hspi1, &command, 1U, 100U) != HAL_OK ||
      HAL_SPI_Receive(&hspi1, id, sizeof(id), 100U) != HAL_OK) {
    EEPROM_FlashDeselect();
    return EEPROM_ERROR;
  }
  EEPROM_FlashDeselect();

  if (id[0] != EEPROM_FLASH_EXPECTED_MFG || id[2] != EEPROM_FLASH_EXPECTED_CAPACITY) {
    return EEPROM_ERROR;
  }

  return EEPROM_OK;
}
