/**
 ******************************************************************************
 * @file           : eeprom.c
 * @brief          : Persistent storage implementation on W25Q16 SPI flash
 ******************************************************************************
 */

#include "main.h"
#include <stdio.h>
#include <string.h>

extern SPI_HandleTypeDef hspi1;

static eeprom_handle_t h_eeprom = {0};
static uint8_t g_eeprom_sector_cache[4096] = {0};

typedef struct {
  uint8_t enabled;
  uint16_t start_hhmm;
  uint16_t end_hhmm;
  uint8_t valve_mask;
  uint16_t irrigation_min;
  uint16_t wait_min;
  uint8_t repeat_count;
  uint8_t days_mask;
  int16_t ec_set_x100;
  int16_t ph_set_x100;
  uint8_t fert_ratio_percent[IRRIGATION_EC_CHANNEL_COUNT];
  uint16_t last_run_day;
  uint16_t last_run_minute;
  uint16_t crc;
} legacy_recipe_irrigation_program_t;

typedef struct {
  uint8_t enabled;
  uint16_t start_hhmm;
  uint16_t end_hhmm;
  uint8_t valve_mask;
  uint16_t irrigation_min;
  uint16_t wait_min;
  uint8_t repeat_count;
  uint8_t days_mask;
  int16_t ec_set_x100;
  int16_t ph_set_x100;
  uint16_t last_run_day;
  uint16_t last_run_minute;
  uint8_t valve_duration_min[IRRIGATION_PROGRAM_VALVE_COUNT];
  uint16_t crc;
} compat_irrigation_program_t;

typedef struct {
  uint8_t enabled;
  uint16_t start_hhmm;
  uint16_t end_hhmm;
  uint8_t valve_mask;
  uint16_t irrigation_min;
  uint16_t wait_min;
  uint8_t repeat_count;
  uint8_t days_mask;
  int16_t ec_set_x100;
  int16_t ph_set_x100;
  uint8_t fert_ratio_percent[IRRIGATION_EC_CHANNEL_COUNT];
  uint16_t pre_flush_sec;
  uint16_t post_flush_sec;
  uint16_t last_run_day;
  uint16_t last_run_minute;
  irrigation_trigger_mode_t trigger_mode;
  int16_t anchor_offset_min;
  uint16_t period_min;
  uint8_t max_events_per_day;
  uint16_t crc;
} legacy_trigger_irrigation_program_t;

typedef struct {
  uint8_t enabled;
  uint16_t start_hhmm;
  uint16_t end_hhmm;
  uint8_t valve_mask;
  uint16_t irrigation_min;
  uint16_t wait_min;
  uint8_t repeat_count;
  uint8_t days_mask;
  int16_t ec_set_x100;
  int16_t ph_set_x100;
  uint16_t last_run_day;
  uint16_t last_run_minute;
  uint8_t repeat_interval_min;
  uint8_t valve_duration_min[IRRIGATION_PROGRAM_VALVE_COUNT];
  uint16_t crc;
} legacy_irrigation_program_t;

typedef struct {
  uint8_t enabled;
  uint16_t start_hhmm;
  uint16_t end_hhmm;
  uint8_t valve_mask;
  uint16_t irrigation_min;
  uint16_t wait_min;
  uint8_t repeat_count;
  uint8_t days_mask;
  int16_t ec_set_x100;
  int16_t ph_set_x100;
  uint16_t last_run_day;
  uint16_t last_run_minute;
  uint8_t reserved[4];
  uint16_t crc;
} legacy_basic_irrigation_program_t;

static uint8_t EEPROM_IsStructCRCValid(void *data, uint16_t size,
                                       uint16_t expected_crc);
static uint8_t EEPROM_IsHeaderValid(const eeprom_header_t *header);
static uint8_t EEPROM_IsSystemParamsSane(const eeprom_system_t *system);
static uint8_t EEPROM_IsParcelRecordSane(const eeprom_parcel_t *parcel);
static void EEPROM_SetDefaultSystemParams(void);
static void EEPROM_SetDefaultPrograms(void);
static void EEPROM_SetDefaultParcel(uint8_t parcel_id, eeprom_parcel_t *parcel);
static uint16_t EEPROM_GetProgramAddress(uint8_t program_id);
static uint16_t EEPROM_GetLegacyTriggerProgramAddress(uint8_t program_id);
static uint16_t EEPROM_GetLegacyRecipeProgramAddress(uint8_t program_id);
static uint16_t EEPROM_GetCompatProgramAddress(uint8_t program_id);
static uint16_t EEPROM_GetLegacyProgramAddress(uint8_t program_id);
static uint16_t EEPROM_GetLegacyBasicProgramAddress(uint8_t program_id);
static void EEPROM_MigrateLegacyRecipeProgram(
    const legacy_recipe_irrigation_program_t *legacy,
    irrigation_program_t *program);
static void EEPROM_MigrateLegacyTriggerProgram(
    const legacy_trigger_irrigation_program_t *legacy,
    irrigation_program_t *program);
static void EEPROM_MigrateCompatProgram(const compat_irrigation_program_t *legacy,
                                        irrigation_program_t *program);
static void EEPROM_MigrateLegacyProgram(
    const legacy_irrigation_program_t *legacy,
    irrigation_program_t *program);
static void EEPROM_MigrateLegacyBasicProgram(
    const legacy_basic_irrigation_program_t *legacy,
    irrigation_program_t *program);
static uint8_t EEPROM_LoadCompatPrograms(irrigation_program_t *programs,
                                         uint8_t count);
static uint8_t EEPROM_LoadLegacyTriggerPrograms(irrigation_program_t *programs,
                                                uint8_t count);
static uint8_t EEPROM_LoadLegacyRecipePrograms(irrigation_program_t *programs,
                                               uint8_t count);
static uint8_t EEPROM_LoadLegacyPrograms(irrigation_program_t *programs,
                                         uint8_t count);
static uint8_t EEPROM_LoadLegacyBasicPrograms(irrigation_program_t *programs,
                                              uint8_t count);
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

_Static_assert((sizeof(irrigation_program_t) * IRRIGATION_PROGRAM_COUNT) <=
                   (EEPROM_ADDR_RUNTIME_BACKUP - EEPROM_ADDR_PROGRAMS),
               "Program storage overlaps runtime backup area");

uint8_t EEPROM_Init(void) {
  if (EEPROM_FlashInit() != EEPROM_OK) {
    h_eeprom.last_error = EEPROM_NOT_INITIALIZED;
    return EEPROM_ERROR;
  }

  if (EEPROM_ReadBuffer(EEPROM_ADDR_MAGIC, (uint8_t *)&h_eeprom.header,
                        sizeof(h_eeprom.header)) != EEPROM_OK) {
    return EEPROM_ERROR;
  }

  if (EEPROM_IsHeaderValid(&h_eeprom.header) == 0U) {
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
                               h_eeprom.system.crc) ||
      EEPROM_IsSystemParamsSane(&h_eeprom.system) == 0U) {
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

uint8_t EEPROM_LoadParcel(uint8_t parcel_id) {
  eeprom_parcel_t record = {0};

  if (parcel_id == 0U || parcel_id > PARCEL_VALVE_COUNT) {
    h_eeprom.last_error = EEPROM_READ_ERROR;
    return EEPROM_ERROR;
  }

  if (EEPROM_ReadBuffer((uint16_t)(EEPROM_ADDR_PARCELS +
                                   ((uint16_t)(parcel_id - 1U) *
                                    sizeof(eeprom_parcel_t))),
                        (uint8_t *)&record, sizeof(record)) != EEPROM_OK) {
    return EEPROM_ERROR;
  }

  if (!EEPROM_IsStructCRCValid(&record, sizeof(record), record.crc) ||
      EEPROM_IsParcelRecordSane(&record) == 0U) {
    EEPROM_SetDefaultParcel(parcel_id, &record);
    h_eeprom.parcels[parcel_id - 1U] = record;
    (void)EEPROM_SaveParcel(parcel_id);
    h_eeprom.last_error = EEPROM_CRC_ERROR;
    return EEPROM_ERROR;
  }

  h_eeprom.parcels[parcel_id - 1U] = record;
  PARCELS_SetName(parcel_id, record.name);
  PARCELS_SetDuration(parcel_id, record.duration_sec);
  PARCELS_SetEnabled(parcel_id, record.enabled);
  return EEPROM_OK;
}

uint8_t EEPROM_SaveParcel(uint8_t parcel_id) {
  eeprom_parcel_t record = {0};

  if (parcel_id == 0U || parcel_id > PARCEL_VALVE_COUNT) {
    h_eeprom.last_error = EEPROM_WRITE_ERROR;
    return EEPROM_ERROR;
  }

  (void)snprintf(record.name, sizeof(record.name), "%s",
                 PARCELS_GetName(parcel_id));
  record.duration_sec = PARCELS_GetDuration(parcel_id);
  record.wait_sec = 0U;
  record.enabled = PARCELS_IsEnabled(parcel_id);

  if (EEPROM_IsParcelRecordSane(&record) == 0U) {
    EEPROM_SetDefaultParcel(parcel_id, &record);
  }

  record.crc = EEPROM_CalculateCRC(&record, sizeof(record) - 2U);
  h_eeprom.parcels[parcel_id - 1U] = record;
  return EEPROM_WriteBuffer((uint16_t)(EEPROM_ADDR_PARCELS +
                                       ((uint16_t)(parcel_id - 1U) *
                                        sizeof(eeprom_parcel_t))),
                            (uint8_t *)&record, sizeof(record));
}

uint8_t EEPROM_LoadAllParcels(void) {
  uint8_t all_valid = 1U;

  for (uint8_t i = 1U; i <= PARCEL_VALVE_COUNT; i++) {
    if (EEPROM_LoadParcel(i) != EEPROM_OK) {
      eeprom_parcel_t *record = &h_eeprom.parcels[i - 1U];
      PARCELS_SetName(i, record->name);
      PARCELS_SetDuration(i, record->duration_sec);
      PARCELS_SetEnabled(i, record->enabled);
      all_valid = 0U;
    }
  }

  return (all_valid != 0U) ? EEPROM_OK : EEPROM_ERROR;
}

uint8_t EEPROM_SaveAllParcels(void) {
  uint8_t all_saved = 1U;

  for (uint8_t i = 1U; i <= PARCEL_VALVE_COUNT; i++) {
    if (EEPROM_SaveParcel(i) != EEPROM_OK) {
      all_saved = 0U;
    }
  }

  return (all_saved != 0U) ? EEPROM_OK : EEPROM_ERROR;
}

uint8_t EEPROM_LoadProgram(uint8_t program_id, irrigation_program_t *program) {
  irrigation_program_t record = {0};

  if (program_id == 0U || program_id > IRRIGATION_PROGRAM_COUNT || program == NULL) {
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
  uint8_t result;

  if (program_id == 0U || program_id > IRRIGATION_PROGRAM_COUNT || program == NULL) {
    h_eeprom.last_error = EEPROM_WRITE_ERROR;
    return EEPROM_ERROR;
  }

  record = *program;
  record.crc = EEPROM_CalculateCRC(&record, sizeof(record) - 2U);
  result = EEPROM_WriteBuffer(EEPROM_GetProgramAddress(program_id),
                              (uint8_t *)&record, sizeof(record));
  if (result == EEPROM_OK) {
    h_eeprom.last_error = EEPROM_OK;
  }

  return result;
}

uint8_t EEPROM_LoadAllPrograms(irrigation_program_t *programs, uint8_t count) {
  uint8_t max_count =
      (count > IRRIGATION_PROGRAM_COUNT) ? IRRIGATION_PROGRAM_COUNT : count;
  uint8_t all_valid = 1U;

  if (programs == NULL) {
    return EEPROM_ERROR;
  }

  for (uint8_t i = 0U; i < max_count; i++) {
    if (EEPROM_LoadProgram(i + 1U, &programs[i]) != EEPROM_OK) {
      memset(&programs[i], 0, sizeof(programs[i]));
      all_valid = 0U;
    }
  }

  if (all_valid != 0U) {
    h_eeprom.last_error = EEPROM_OK;
    return EEPROM_OK;
  }

  if (EEPROM_LoadLegacyTriggerPrograms(programs, max_count) == EEPROM_OK) {
    for (uint8_t i = 0U; i < max_count; i++) {
      (void)EEPROM_SaveProgram(i + 1U, &programs[i]);
    }
    h_eeprom.last_error = EEPROM_OK;
    return EEPROM_OK;
  }

  if (EEPROM_LoadLegacyRecipePrograms(programs, max_count) == EEPROM_OK) {
    for (uint8_t i = 0U; i < max_count; i++) {
      (void)EEPROM_SaveProgram(i + 1U, &programs[i]);
    }
    h_eeprom.last_error = EEPROM_OK;
    return EEPROM_OK;
  }

  if (EEPROM_LoadCompatPrograms(programs, max_count) == EEPROM_OK) {
    for (uint8_t i = 0U; i < max_count; i++) {
      (void)EEPROM_SaveProgram(i + 1U, &programs[i]);
    }
    h_eeprom.last_error = EEPROM_OK;
    return EEPROM_OK;
  }

  if (EEPROM_LoadLegacyPrograms(programs, max_count) == EEPROM_OK) {
    for (uint8_t i = 0U; i < max_count; i++) {
      (void)EEPROM_SaveProgram(i + 1U, &programs[i]);
    }
    h_eeprom.last_error = EEPROM_OK;
    return EEPROM_OK;
  }

  if (EEPROM_LoadLegacyBasicPrograms(programs, max_count) == EEPROM_OK) {
    for (uint8_t i = 0U; i < max_count; i++) {
      (void)EEPROM_SaveProgram(i + 1U, &programs[i]);
    }
    h_eeprom.last_error = EEPROM_OK;
    return EEPROM_OK;
  }

  h_eeprom.last_error = EEPROM_CRC_ERROR;
  return EEPROM_ERROR;
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

  if (record.valid == 0U) {
    memset(runtime_backup, 0, sizeof(*runtime_backup));
    h_eeprom.last_error = EEPROM_OK;
    return EEPROM_OK;
  }

  if (!EEPROM_IsStructCRCValid(&record, sizeof(record), record.crc)) {
    memset(runtime_backup, 0, sizeof(*runtime_backup));
    h_eeprom.last_error = EEPROM_CRC_ERROR;
    return EEPROM_ERROR;
  }

  *runtime_backup = record;
  h_eeprom.last_error = EEPROM_OK;
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
  if (data == NULL || size == 0U) {
    return 0U;
  }

  return CRC16_Calculate((const uint8_t *)data, size);
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
  h_eeprom.system.ph_feedback_delay_ms = IRRIGATION_SETTLING_TIME * 1000U;
  h_eeprom.system.ec_feedback_delay_ms = IRRIGATION_SETTLING_TIME * 1000U;
  h_eeprom.system.ph_response_gain_percent = 100U;
  h_eeprom.system.ec_response_gain_percent = 100U;
  h_eeprom.system.ph_max_correction_cycles = 3U;
  h_eeprom.system.ec_max_correction_cycles = 3U;
  h_eeprom.system.reserved[0] = AUTO_MODE_SCHEDULED;
  h_eeprom.system.reserved[1] = EEPROM_SYSTEM_MODE_MARKER;
  h_eeprom.system.reserved[2] = EEPROM_SYSTEM_SCHEMA_VERSION;
}

static void EEPROM_SetDefaultPrograms(void) {
  irrigation_program_t program = {0};

  for (uint8_t i = 0U; i < IRRIGATION_PROGRAM_COUNT; i++) {
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
    for (uint8_t ratio_idx = 0U; ratio_idx < IRRIGATION_EC_CHANNEL_COUNT;
         ratio_idx++) {
      program.fert_ratio_percent[ratio_idx] = 25U;
    }
    program.pre_flush_sec = IRRIGATION_DEFAULT_PRE_FLUSH_SEC;
    program.post_flush_sec = IRRIGATION_DEFAULT_POST_FLUSH_SEC;
    program.last_run_day = 0U;
    program.last_run_minute = 0xFFFFU;
    program.trigger_mode = IRRIGATION_TRIGGER_FIXED_WINDOW;
    program.anchor_offset_min = 0;
    program.period_min = 120U;
    program.max_events_per_day = 1U;
    program.learned_ph_pwm_percent = 0U;
    program.learned_ec_pwm_percent = 0U;
    (void)EEPROM_SaveProgram(i + 1U, &program);
  }

  for (uint8_t i = 1U; i <= PARCEL_VALVE_COUNT; i++) {
    eeprom_parcel_t record = {0};

    EEPROM_SetDefaultParcel(i, &record);
    h_eeprom.parcels[i - 1U] = record;
    (void)EEPROM_WriteBuffer((uint16_t)(EEPROM_ADDR_PARCELS +
                                        ((uint16_t)(i - 1U) *
                                         sizeof(eeprom_parcel_t))),
                             (uint8_t *)&record, sizeof(record));
  }
}

static uint8_t EEPROM_IsHeaderValid(const eeprom_header_t *header) {
  if (header == NULL) {
    return 0U;
  }

  if (header->magic != EEPROM_MAGIC_VALUE || header->initialized == 0U) {
    return 0U;
  }

  return (header->version_major == FIRMWARE_VERSION_MAJOR) ? 1U : 0U;
}

static uint8_t EEPROM_IsSystemParamsSane(const eeprom_system_t *system) {
  if (system == NULL) {
    return 0U;
  }

  if (system->reserved[2] != EEPROM_SYSTEM_SCHEMA_VERSION) {
    return 0U;
  }

  if (system->brightness > 100U) {
    return 0U;
  }

  if (system->ph_min < 0.0f || system->ph_min >= system->ph_max ||
      system->ph_target < system->ph_min || system->ph_target > system->ph_max ||
      system->ph_max > 14.0f) {
    return 0U;
  }

  if (system->ec_min < 0.0f || system->ec_min >= system->ec_max ||
      system->ec_target < system->ec_min || system->ec_target > system->ec_max ||
      system->ec_max > 10.0f) {
    return 0U;
  }

  if (system->irrigation_interval == 0U ||
      system->irrigation_interval > (24UL * 60UL)) {
    return 0U;
  }

  if (system->ph_feedback_delay_ms > 600000UL ||
      system->ec_feedback_delay_ms > 600000UL) {
    return 0U;
  }

  if (system->ph_response_gain_percent == 0U ||
      system->ph_response_gain_percent > 100U ||
      system->ec_response_gain_percent == 0U ||
      system->ec_response_gain_percent > 100U) {
    return 0U;
  }

  if ((system->system_flags & (uint16_t)~EEPROM_SYSTEM_FLAGS_KNOWN_MASK) != 0U) {
    return 0U;
  }

  if (system->ph_max_correction_cycles > 10U ||
      system->ec_max_correction_cycles > 10U) {
    return 0U;
  }

  if (system->reserved[1] == EEPROM_SYSTEM_MODE_MARKER &&
      system->reserved[0] > (uint8_t)AUTO_MODE_SCHEDULED) {
    return 0U;
  }

  return 1U;
}

static uint8_t EEPROM_IsParcelRecordSane(const eeprom_parcel_t *parcel) {
  if (parcel == NULL) {
    return 0U;
  }

  if (parcel->name[sizeof(parcel->name) - 1U] != '\0') {
    return 0U;
  }

  if (parcel->duration_sec < 30U || parcel->duration_sec > (24UL * 60UL * 60UL)) {
    return 0U;
  }

  if (parcel->enabled > 1U) {
    return 0U;
  }

  return 1U;
}

static void EEPROM_SetDefaultParcel(uint8_t parcel_id, eeprom_parcel_t *parcel) {
  if (parcel == NULL || parcel_id == 0U || parcel_id > PARCEL_VALVE_COUNT) {
    return;
  }

  memset(parcel, 0, sizeof(*parcel));
  (void)snprintf(parcel->name, sizeof(parcel->name), "Parcel %u", parcel_id);
  parcel->duration_sec = 300U;
  parcel->wait_sec = 0U;
  parcel->enabled = 1U;
  parcel->crc = EEPROM_CalculateCRC(parcel, sizeof(*parcel) - 2U);
}

static uint16_t EEPROM_GetProgramAddress(uint8_t program_id) {
  return (uint16_t)(EEPROM_ADDR_PROGRAMS +
                    ((uint16_t)(program_id - 1U) * sizeof(irrigation_program_t)));
}

static uint16_t EEPROM_GetLegacyTriggerProgramAddress(uint8_t program_id) {
  return (uint16_t)(
      EEPROM_ADDR_PROGRAMS +
      ((uint16_t)(program_id - 1U) * sizeof(legacy_trigger_irrigation_program_t)));
}

static uint16_t EEPROM_GetLegacyRecipeProgramAddress(uint8_t program_id) {
  return (uint16_t)(EEPROM_ADDR_PROGRAMS +
                    ((uint16_t)(program_id - 1U) *
                     sizeof(legacy_recipe_irrigation_program_t)));
}

static uint16_t EEPROM_GetCompatProgramAddress(uint8_t program_id) {
  return (uint16_t)(EEPROM_ADDR_PROGRAMS +
                    ((uint16_t)(program_id - 1U) *
                     sizeof(compat_irrigation_program_t)));
}

static uint16_t EEPROM_GetLegacyProgramAddress(uint8_t program_id) {
  return (uint16_t)(EEPROM_ADDR_PROGRAMS + ((uint16_t)(program_id - 1U) *
                                            sizeof(legacy_irrigation_program_t)));
}

static uint16_t EEPROM_GetLegacyBasicProgramAddress(uint8_t program_id) {
  return (uint16_t)(
      EEPROM_ADDR_PROGRAMS +
      ((uint16_t)(program_id - 1U) * sizeof(legacy_basic_irrigation_program_t)));
}

static void EEPROM_MigrateLegacyRecipeProgram(
    const legacy_recipe_irrigation_program_t *legacy,
    irrigation_program_t *program) {
  if (legacy == NULL || program == NULL) {
    return;
  }

  memset(program, 0, sizeof(*program));
  program->enabled = legacy->enabled;
  program->start_hhmm = legacy->start_hhmm;
  program->end_hhmm = legacy->end_hhmm;
  program->valve_mask = legacy->valve_mask;
  program->irrigation_min = legacy->irrigation_min;
  program->wait_min = legacy->wait_min;
  program->repeat_count = legacy->repeat_count;
  program->days_mask = legacy->days_mask;
  program->ec_set_x100 = legacy->ec_set_x100;
  program->ph_set_x100 = legacy->ph_set_x100;
  memcpy(program->fert_ratio_percent, legacy->fert_ratio_percent,
         sizeof(program->fert_ratio_percent));
  program->pre_flush_sec = IRRIGATION_DEFAULT_PRE_FLUSH_SEC;
  program->post_flush_sec = IRRIGATION_DEFAULT_POST_FLUSH_SEC;
  program->last_run_day = legacy->last_run_day;
  program->last_run_minute = legacy->last_run_minute;
  program->trigger_mode = IRRIGATION_TRIGGER_FIXED_WINDOW;
  program->anchor_offset_min = 0;
  program->period_min = 120U;
  program->max_events_per_day = 1U;
  program->learned_ph_pwm_percent = 0U;
  program->learned_ec_pwm_percent = 0U;
}

static void EEPROM_MigrateLegacyTriggerProgram(
    const legacy_trigger_irrigation_program_t *legacy,
    irrigation_program_t *program) {
  if (legacy == NULL || program == NULL) {
    return;
  }

  memset(program, 0, sizeof(*program));
  program->enabled = legacy->enabled;
  program->start_hhmm = legacy->start_hhmm;
  program->end_hhmm = legacy->end_hhmm;
  program->valve_mask = legacy->valve_mask;
  program->irrigation_min = legacy->irrigation_min;
  program->wait_min = legacy->wait_min;
  program->repeat_count = legacy->repeat_count;
  program->days_mask = legacy->days_mask;
  program->ec_set_x100 = legacy->ec_set_x100;
  program->ph_set_x100 = legacy->ph_set_x100;
  memcpy(program->fert_ratio_percent, legacy->fert_ratio_percent,
         sizeof(program->fert_ratio_percent));
  program->pre_flush_sec = legacy->pre_flush_sec;
  program->post_flush_sec = legacy->post_flush_sec;
  program->last_run_day = legacy->last_run_day;
  program->last_run_minute = legacy->last_run_minute;
  program->trigger_mode = legacy->trigger_mode;
  program->anchor_offset_min = legacy->anchor_offset_min;
  program->period_min = legacy->period_min;
  program->max_events_per_day = legacy->max_events_per_day;
  program->learned_ph_pwm_percent = 0U;
  program->learned_ec_pwm_percent = 0U;
}

static void EEPROM_MigrateCompatProgram(const compat_irrigation_program_t *legacy,
                                        irrigation_program_t *program) {
  if (legacy == NULL || program == NULL) {
    return;
  }

  memset(program, 0, sizeof(*program));
  program->enabled = legacy->enabled;
  program->start_hhmm = legacy->start_hhmm;
  program->end_hhmm = legacy->end_hhmm;
  program->valve_mask = legacy->valve_mask;
  program->irrigation_min = legacy->irrigation_min;
  program->wait_min = legacy->wait_min;
  program->repeat_count = legacy->repeat_count;
  program->days_mask = legacy->days_mask;
  program->ec_set_x100 = legacy->ec_set_x100;
  program->ph_set_x100 = legacy->ph_set_x100;
  for (uint8_t i = 0U; i < IRRIGATION_EC_CHANNEL_COUNT; i++) {
    program->fert_ratio_percent[i] = 25U;
  }
  program->pre_flush_sec = IRRIGATION_DEFAULT_PRE_FLUSH_SEC;
  program->post_flush_sec = IRRIGATION_DEFAULT_POST_FLUSH_SEC;
  program->last_run_day = legacy->last_run_day;
  program->last_run_minute = legacy->last_run_minute;
  program->trigger_mode = IRRIGATION_TRIGGER_FIXED_WINDOW;
  program->anchor_offset_min = 0;
  program->period_min = 120U;
  program->max_events_per_day = 1U;
  program->learned_ph_pwm_percent = 0U;
  program->learned_ec_pwm_percent = 0U;
}

static void EEPROM_MigrateLegacyProgram(
    const legacy_irrigation_program_t *legacy,
    irrigation_program_t *program) {
  if (legacy == NULL || program == NULL) {
    return;
  }

  memset(program, 0, sizeof(*program));
  program->enabled = legacy->enabled;
  program->start_hhmm = legacy->start_hhmm;
  program->end_hhmm = legacy->end_hhmm;
  program->valve_mask = legacy->valve_mask;
  program->irrigation_min = legacy->irrigation_min;
  program->wait_min = legacy->wait_min;
  program->repeat_count = legacy->repeat_count;
  program->days_mask = legacy->days_mask;
  program->ec_set_x100 = legacy->ec_set_x100;
  program->ph_set_x100 = legacy->ph_set_x100;
  for (uint8_t i = 0U; i < IRRIGATION_EC_CHANNEL_COUNT; i++) {
    program->fert_ratio_percent[i] = 25U;
  }
  program->pre_flush_sec = IRRIGATION_DEFAULT_PRE_FLUSH_SEC;
  program->post_flush_sec = IRRIGATION_DEFAULT_POST_FLUSH_SEC;
  program->last_run_day = legacy->last_run_day;
  program->last_run_minute = legacy->last_run_minute;
  program->trigger_mode = IRRIGATION_TRIGGER_FIXED_WINDOW;
  program->anchor_offset_min = 0;
  program->period_min = 120U;
  program->max_events_per_day = 1U;
  program->learned_ph_pwm_percent = 0U;
  program->learned_ec_pwm_percent = 0U;

  for (uint8_t i = 0U; i < IRRIGATION_PROGRAM_VALVE_COUNT; i++) {
    if ((program->valve_mask & (1U << i)) == 0U ||
        legacy->valve_duration_min[i] == 0U) {
      continue;
    }

    program->irrigation_min = legacy->valve_duration_min[i];
    break;
  }
}

static void EEPROM_MigrateLegacyBasicProgram(
    const legacy_basic_irrigation_program_t *legacy,
    irrigation_program_t *program) {
  if (legacy == NULL || program == NULL) {
    return;
  }

  memset(program, 0, sizeof(*program));
  program->enabled = legacy->enabled;
  program->start_hhmm = legacy->start_hhmm;
  program->end_hhmm = legacy->end_hhmm;
  program->valve_mask = legacy->valve_mask;
  program->irrigation_min = legacy->irrigation_min;
  program->wait_min = legacy->wait_min;
  program->repeat_count = legacy->repeat_count;
  program->days_mask = legacy->days_mask;
  program->ec_set_x100 = legacy->ec_set_x100;
  program->ph_set_x100 = legacy->ph_set_x100;
  for (uint8_t i = 0U; i < IRRIGATION_EC_CHANNEL_COUNT; i++) {
    program->fert_ratio_percent[i] = 25U;
  }
  program->pre_flush_sec = IRRIGATION_DEFAULT_PRE_FLUSH_SEC;
  program->post_flush_sec = IRRIGATION_DEFAULT_POST_FLUSH_SEC;
  program->last_run_day = legacy->last_run_day;
  program->last_run_minute = legacy->last_run_minute;
  program->trigger_mode = IRRIGATION_TRIGGER_FIXED_WINDOW;
  program->anchor_offset_min = 0;
  program->period_min = 120U;
  program->max_events_per_day = 1U;
  program->learned_ph_pwm_percent = 0U;
  program->learned_ec_pwm_percent = 0U;
}

static uint8_t EEPROM_LoadLegacyTriggerPrograms(irrigation_program_t *programs,
                                                uint8_t count) {
  uint8_t max_count =
      (count > IRRIGATION_PROGRAM_COUNT) ? IRRIGATION_PROGRAM_COUNT : count;

  if (programs == NULL) {
    return EEPROM_ERROR;
  }

  for (uint8_t i = 0U; i < max_count; i++) {
    legacy_trigger_irrigation_program_t legacy = {0};

    if (EEPROM_ReadBuffer(EEPROM_GetLegacyTriggerProgramAddress(i + 1U),
                          (uint8_t *)&legacy, sizeof(legacy)) != EEPROM_OK) {
      return EEPROM_ERROR;
    }

    if (!EEPROM_IsStructCRCValid(&legacy, sizeof(legacy), legacy.crc)) {
      return EEPROM_ERROR;
    }

    EEPROM_MigrateLegacyTriggerProgram(&legacy, &programs[i]);
  }

  return EEPROM_OK;
}

static uint8_t EEPROM_LoadLegacyRecipePrograms(irrigation_program_t *programs,
                                               uint8_t count) {
  uint8_t max_count =
      (count > IRRIGATION_PROGRAM_COUNT) ? IRRIGATION_PROGRAM_COUNT : count;

  if (programs == NULL) {
    return EEPROM_ERROR;
  }

  for (uint8_t i = 0U; i < max_count; i++) {
    legacy_recipe_irrigation_program_t legacy = {0};

    if (EEPROM_ReadBuffer(EEPROM_GetLegacyRecipeProgramAddress(i + 1U),
                          (uint8_t *)&legacy, sizeof(legacy)) != EEPROM_OK) {
      return EEPROM_ERROR;
    }

    if (!EEPROM_IsStructCRCValid(&legacy, sizeof(legacy), legacy.crc)) {
      return EEPROM_ERROR;
    }

    EEPROM_MigrateLegacyRecipeProgram(&legacy, &programs[i]);
  }

  return EEPROM_OK;
}

static uint8_t EEPROM_LoadCompatPrograms(irrigation_program_t *programs,
                                         uint8_t count) {
  uint8_t max_count =
      (count > IRRIGATION_PROGRAM_COUNT) ? IRRIGATION_PROGRAM_COUNT : count;

  if (programs == NULL) {
    return EEPROM_ERROR;
  }

  for (uint8_t i = 0U; i < max_count; i++) {
    compat_irrigation_program_t legacy = {0};

    if (EEPROM_ReadBuffer(EEPROM_GetCompatProgramAddress(i + 1U),
                          (uint8_t *)&legacy, sizeof(legacy)) != EEPROM_OK) {
      return EEPROM_ERROR;
    }

    if (!EEPROM_IsStructCRCValid(&legacy, sizeof(legacy), legacy.crc)) {
      return EEPROM_ERROR;
    }

    EEPROM_MigrateCompatProgram(&legacy, &programs[i]);
  }

  return EEPROM_OK;
}

static uint8_t EEPROM_LoadLegacyPrograms(irrigation_program_t *programs,
                                         uint8_t count) {
  uint8_t max_count =
      (count > IRRIGATION_PROGRAM_COUNT) ? IRRIGATION_PROGRAM_COUNT : count;

  if (programs == NULL) {
    return EEPROM_ERROR;
  }

  for (uint8_t i = 0U; i < max_count; i++) {
    legacy_irrigation_program_t legacy = {0};

    if (EEPROM_ReadBuffer(EEPROM_GetLegacyProgramAddress(i + 1U),
                          (uint8_t *)&legacy, sizeof(legacy)) != EEPROM_OK) {
      return EEPROM_ERROR;
    }

    if (!EEPROM_IsStructCRCValid(&legacy, sizeof(legacy), legacy.crc)) {
      return EEPROM_ERROR;
    }

    EEPROM_MigrateLegacyProgram(&legacy, &programs[i]);
  }

  return EEPROM_OK;
}

static uint8_t EEPROM_LoadLegacyBasicPrograms(irrigation_program_t *programs,
                                              uint8_t count) {
  uint8_t max_count =
      (count > IRRIGATION_PROGRAM_COUNT) ? IRRIGATION_PROGRAM_COUNT : count;

  if (programs == NULL) {
    return EEPROM_ERROR;
  }

  for (uint8_t i = 0U; i < max_count; i++) {
    legacy_basic_irrigation_program_t legacy = {0};

    if (EEPROM_ReadBuffer(EEPROM_GetLegacyBasicProgramAddress(i + 1U),
                          (uint8_t *)&legacy, sizeof(legacy)) != EEPROM_OK) {
      return EEPROM_ERROR;
    }

    if (!EEPROM_IsStructCRCValid(&legacy, sizeof(legacy), legacy.crc)) {
      return EEPROM_ERROR;
    }

    EEPROM_MigrateLegacyBasicProgram(&legacy, &programs[i]);
  }

  return EEPROM_OK;
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
