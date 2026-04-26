/**
 ******************************************************************************
 * @file           : eeprom.h
 * @brief          : EEPROM 24C256 Veri Saklama Driver
 ******************************************************************************
 */

#ifndef __EEPROM_H
#define __EEPROM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "irrigation_types.h"
#include "touch_xpt2046.h"

/* EEPROM Configuration -----------------------------------------------------*/
#define EEPROM_I2C I2C1
#define EEPROM_ADDRESS 0x50U     /* 24C256 I2C Address */
#define EEPROM_PAGE_SIZE 64U     /* 64 Byte page size */
#define EEPROM_TOTAL_SIZE 32768U /* 32 KBit = 4 KByte */
#define EEPROM_SYSTEM_MODE_MARKER 0xA5U
#define EEPROM_SYSTEM_SCHEMA_VERSION 2U
#define EEPROM_SYSTEM_FLAG_DOSING_LINEAR 0x0001U
#define EEPROM_SYSTEM_FLAG_DOSING_ACID_DISABLED 0x0002U
#define EEPROM_SYSTEM_FLAG_DOSING_FERT_A_DISABLED 0x0004U
#define EEPROM_SYSTEM_FLAG_DOSING_FERT_B_DISABLED 0x0008U
#define EEPROM_SYSTEM_FLAG_DOSING_FERT_C_DISABLED 0x0010U
#define EEPROM_SYSTEM_FLAG_DOSING_FERT_D_DISABLED 0x0020U
#define EEPROM_SYSTEM_FLAGS_KNOWN_MASK \
  (EEPROM_SYSTEM_FLAG_DOSING_LINEAR | EEPROM_SYSTEM_FLAG_DOSING_ACID_DISABLED | \
   EEPROM_SYSTEM_FLAG_DOSING_FERT_A_DISABLED | \
   EEPROM_SYSTEM_FLAG_DOSING_FERT_B_DISABLED | \
   EEPROM_SYSTEM_FLAG_DOSING_FERT_C_DISABLED | \
   EEPROM_SYSTEM_FLAG_DOSING_FERT_D_DISABLED)

/* EEPROM Memory Map --------------------------------------------------------*/
/* Sayfa 0-1: Magic Number ve Versiyon */
#define EEPROM_ADDR_MAGIC 0x0000U
#define EEPROM_MAGIC_VALUE 0x55AAU

/* Sayfa 2-3: Sistem Parametreleri */
#define EEPROM_ADDR_SYSTEM 0x0080U

/* Sayfa 4-7: pH Kalibrasyon Verileri */
#define EEPROM_ADDR_PH_CAL 0x0100U

/* Sayfa 8-11: EC Kalibrasyon Verileri */
#define EEPROM_ADDR_EC_CAL 0x0200U

/* Sayfa 12-15: Parsel Ayarları (8 parsel x 16 byte) */
#define EEPROM_ADDR_PARCELS 0x0300U

/* Sayfa 20-23: Sulama Programlari */
#define EEPROM_ADDR_PROGRAMS 0x0500U

/* Sayfa 26-27: Runtime Backup */
#define EEPROM_ADDR_RUNTIME_BACKUP 0x0700U

/* Sayfa 16-31: Sulama Logları (dairesel buffer) */
#define EEPROM_ADDR_LOGS 0x0800U

/* Sayfa 32-47: Yedek Alan */
#define EEPROM_ADDR_SPARE 0x1000U

/* Sayfa 33: Touch kalibrasyon */
#define EEPROM_ADDR_TOUCH_CAL 0x1080U

/* EEPROM Status ------------------------------------------------------------*/
#define EEPROM_OK 0U
#define EEPROM_ERROR 1U
#define EEPROM_NOT_INITIALIZED 2U
#define EEPROM_WRITE_ERROR 3U
#define EEPROM_READ_ERROR 4U
#define EEPROM_CRC_ERROR 5U

/* Data Structures ----------------------------------------------------------*/
typedef struct {
  uint16_t magic;
  uint8_t version_major;
  uint8_t version_minor;
  uint8_t initialized;
  uint8_t reserved[3];
} eeprom_header_t;

typedef struct {
  uint32_t last_update_time;
  uint16_t system_flags;
  uint8_t language;
  uint8_t brightness;
  float ph_target;
  float ph_min;
  float ph_max;
  float ec_target;
  float ec_min;
  float ec_max;
  uint32_t irrigation_interval; /* dakika */
  uint8_t auto_mode_enabled;
  uint32_t ph_feedback_delay_ms;
  uint32_t ec_feedback_delay_ms;
  uint8_t ph_response_gain_percent;
  uint8_t ec_response_gain_percent;
  uint8_t ph_max_correction_cycles;
  uint8_t ec_max_correction_cycles;
  uint8_t reserved[3];
  uint16_t crc;
} eeprom_system_t;

typedef struct {
  uint32_t calibration_date;
  uint8_t points_count;
  uint8_t is_valid;
  float adc_values[3];
  float ref_values[3];
  float slope;
  float offset;
  uint16_t crc;
} eeprom_ph_cal_t;

typedef struct {
  uint32_t calibration_date;
  uint8_t points_count;
  uint8_t is_valid;
  float adc_values[2];
  float ref_values[2];
  float slope;
  float offset;
  uint16_t crc;
} eeprom_ec_cal_t;

typedef struct {
  char name[16];
  uint32_t duration_sec;
  uint32_t wait_sec;
  uint8_t enabled;
  uint8_t reserved[3];
  uint16_t crc;
} eeprom_parcel_t;

typedef struct {
  uint32_t timestamp;
  uint8_t parcel_id;
  int8_t ph_value_x10;   /* pH * 10 (örn: 65 = 6.5) */
  int16_t ec_value_x100; /* EC * 100 (örn: 150 = 1.50 mS/cm) */
  uint8_t duration_sec;
  uint8_t flags;
} eeprom_log_entry_t;

typedef struct {
  touch_calibration_t calibration;
  uint16_t crc;
} eeprom_touch_cal_t;

typedef struct {
  irrigation_program_t program;
} eeprom_program_t;

typedef struct {
  irrigation_runtime_backup_t backup;
} eeprom_runtime_backup_t;

/* EEPROM Handle ------------------------------------------------------------*/
typedef struct {
  uint8_t initialized;
  uint8_t last_error;
  eeprom_header_t header;
  eeprom_system_t system;
  eeprom_ph_cal_t ph_cal;
  eeprom_ec_cal_t ec_cal;
  eeprom_parcel_t parcels[8];
} eeprom_handle_t;

/* Basic EEPROM Functions ---------------------------------------------------*/
uint8_t EEPROM_Init(void);
uint8_t EEPROM_ReadByte(uint16_t address, uint8_t *data);
uint8_t EEPROM_WriteByte(uint16_t address, uint8_t data);
uint8_t EEPROM_ReadBuffer(uint16_t address, uint8_t *buffer, uint16_t size);
uint8_t EEPROM_WriteBuffer(uint16_t address, uint8_t *buffer, uint16_t size);
uint8_t EEPROM_IsReady(void);
void EEPROM_WaitReady(void);

/* System Parameter Functions -----------------------------------------------*/
uint8_t EEPROM_LoadSystemParams(void);
uint8_t EEPROM_SaveSystemParams(void);
uint8_t EEPROM_ResetSystemParams(void);
uint8_t EEPROM_GetSystemParams(eeprom_system_t *system);
uint8_t EEPROM_SetSystemParams(const eeprom_system_t *system);

/* Calibration Functions ----------------------------------------------------*/
uint8_t EEPROM_LoadPHCalibration(void);
uint8_t EEPROM_SavePHCalibration(void);
uint8_t EEPROM_LoadECCalibration(void);
uint8_t EEPROM_SaveECCalibration(void);
uint8_t EEPROM_LoadTouchCalibration(touch_calibration_t *cal);
uint8_t EEPROM_SaveTouchCalibration(const touch_calibration_t *cal);

/* Parcel Functions ---------------------------------------------------------*/
uint8_t EEPROM_LoadParcel(uint8_t parcel_id);
uint8_t EEPROM_SaveParcel(uint8_t parcel_id);
uint8_t EEPROM_LoadAllParcels(void);
uint8_t EEPROM_SaveAllParcels(void);

/* Program Functions --------------------------------------------------------*/
uint8_t EEPROM_LoadProgram(uint8_t program_id, irrigation_program_t *program);
uint8_t EEPROM_SaveProgram(uint8_t program_id,
                           const irrigation_program_t *program);
uint8_t EEPROM_LoadAllPrograms(irrigation_program_t *programs, uint8_t count);
uint8_t EEPROM_SaveRuntimeBackup(
    const irrigation_runtime_backup_t *runtime_backup);
uint8_t EEPROM_LoadRuntimeBackup(irrigation_runtime_backup_t *runtime_backup);
uint8_t EEPROM_ClearRuntimeBackup(void);

/* Log Functions ------------------------------------------------------------*/
uint8_t EEPROM_WriteLog(eeprom_log_entry_t *entry);
uint8_t EEPROM_ReadLogs(eeprom_log_entry_t *buffer, uint16_t count,
                        uint16_t offset);
uint16_t EEPROM_GetLogCount(void);
uint8_t EEPROM_ClearLogs(void);

/* CRC Functions ------------------------------------------------------------*/
uint16_t EEPROM_CalculateCRC(void *data, uint16_t size);
uint8_t EEPROM_VerifyCRC(void *data, uint16_t size, uint16_t expected_crc);

/* Utility Functions --------------------------------------------------------*/
uint8_t EEPROM_GetLastError(void);
const char *EEPROM_GetErrorString(uint8_t error);
void EEPROM_Format(void); /* Tüm EEPROM'u sil (dikkatli kullanın!) */

#ifdef __cplusplus
}
#endif

#endif /* __EEPROM_H */
