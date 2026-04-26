/**
 ******************************************************************************
 * @file           : irrigation_control.h
 * @brief          : Sulama Algoritması ve Kontrol Mantığı
 ******************************************************************************
 */

#ifndef __IRRIGATION_CONTROL_H
#define __IRRIGATION_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "irrigation_types.h"
#include "sensors.h"
#include "valves.h"
#include <stdbool.h>
#include <stdint.h>

/* Sulama Kontrol Konfigürasyonu --------------------------------------------*/
#define IRRIGATION_PH_HYSTERESIS 0.2f /* pH ölü bant */
#define IRRIGATION_EC_HYSTERESIS 0.3f /* EC ölü bant (mS/cm) */
#define IRRIGATION_MIXING_TIME 30U    /* Karıştırma süresi (saniye) */
#define IRRIGATION_SETTLING_TIME 10U  /* Çökeltme süresi (saniye) */
#define IRRIGATION_SAMPLE_INTERVAL 5U /* Örnek alma aralığı (saniye) */

/* Ana Kontrol Yapısı -------------------------------------------------------*/
typedef struct {
  control_state_t state;
  uint8_t is_running;
  uint8_t is_paused;
  uint8_t error_flags;
  error_code_t last_error;

  ph_control_params_t ph_params;
  ec_control_params_t ec_params;

  ph_sensor_data_t ph_data;
  ec_sensor_data_t ec_data;

  irrigation_timers_t timers;

  parcel_watering_t current_parcel;
  uint8_t parcel_queue[8];
  uint8_t queue_head;
  uint8_t queue_tail;
  uint8_t queue_size;

  uint32_t total_watering_time;
  uint32_t total_parcel_count;
} irrigation_control_t;

/* Kontrol Modları ----------------------------------------------------------*/
/* NOT: irrigation_mode_t valves.h'de tanımlıdır */
/* Bu dosyada farklı modlar için aşağıdaki enum kullanılır */
typedef enum {
  AUTO_MODE_OFF = 0,
  AUTO_MODE_PH_EC_ONLY,
  AUTO_MODE_PARCEL_ONLY,
  AUTO_MODE_FULL_AUTO,
  AUTO_MODE_SCHEDULED
} auto_mode_t;

/* Fonksiyon Prototipleri ---------------------------------------------------*/

/* Initialization -----------------------------------------------------------*/
void IRRIGATION_CTRL_Init(void);
void IRRIGATION_CTRL_SetAutoMode(auto_mode_t mode);
auto_mode_t IRRIGATION_CTRL_GetAutoMode(void);

/* Parameter Setting --------------------------------------------------------*/
void IRRIGATION_CTRL_SetPHParams(float target, float min, float max,
                                 float hyst);
void IRRIGATION_CTRL_SetECParams(float target, float min, float max,
                                 float hyst);
void IRRIGATION_CTRL_SetPHDosingDuty(uint8_t duty_percent);
void IRRIGATION_CTRL_SetECDosingDuty(uint8_t duty_percent);
void IRRIGATION_CTRL_SetPHDosingResponse(uint32_t feedback_delay_ms,
                                         uint8_t response_gain_percent,
                                         uint8_t max_correction_cycles);
void IRRIGATION_CTRL_SetECDosingResponse(uint32_t feedback_delay_ms,
                                         uint8_t response_gain_percent,
                                         uint8_t max_correction_cycles);
void IRRIGATION_CTRL_SetECRecipe(const uint8_t *ratio_percent, uint8_t count);
void IRRIGATION_CTRL_GetECRecipe(uint8_t *ratio_percent, uint8_t count);
void IRRIGATION_CTRL_GetPHParams(ph_control_params_t *params);
void IRRIGATION_CTRL_GetECParams(ec_control_params_t *params);
void IRRIGATION_CTRL_SaveDosingResponseParams(void);
void IRRIGATION_CTRL_SetParcelDuration(uint8_t parcel_id,
                                       uint32_t duration_sec);
void IRRIGATION_CTRL_SetParcelEnabled(uint8_t parcel_id, uint8_t enabled);
void IRRIGATION_CTRL_SetParcelConfig(uint8_t parcel_id, uint32_t duration_sec,
                                     uint8_t enabled);

/* Control Functions --------------------------------------------------------*/
void IRRIGATION_CTRL_Start(void);
void IRRIGATION_CTRL_Stop(void);
void IRRIGATION_CTRL_Pause(void);
void IRRIGATION_CTRL_Resume(void);
uint8_t IRRIGATION_CTRL_IsRunning(void);
void IRRIGATION_CTRL_Update(void); /* Ana kontrol döngüsü - periyodik çağır */

/* State Machine ------------------------------------------------------------*/
control_state_t IRRIGATION_CTRL_GetState(void);
const char *IRRIGATION_CTRL_GetStateName(control_state_t state);

/* Sensor Reading -----------------------------------------------------------*/
void IRRIGATION_CTRL_ReadSensors(void);
float IRRIGATION_CTRL_GetPH(void);
float IRRIGATION_CTRL_GetEC(void);
uint8_t IRRIGATION_CTRL_IsPHValid(void);
uint8_t IRRIGATION_CTRL_IsECValid(void);

/* pH Control ---------------------------------------------------------------*/
uint8_t IRRIGATION_CTRL_CheckPH(void);
void IRRIGATION_CTRL_AdjustPH(void);
uint8_t IRRIGATION_CTRL_IsPHInRange(void);

/* EC Control ---------------------------------------------------------------*/
uint8_t IRRIGATION_CTRL_CheckEC(void);
void IRRIGATION_CTRL_AdjustEC(void);
uint8_t IRRIGATION_CTRL_IsECInRange(void);

/* Parcel Control -----------------------------------------------------------*/
void IRRIGATION_CTRL_StartParcel(uint8_t parcel_id);
void IRRIGATION_CTRL_StopParcel(void);
void IRRIGATION_CTRL_NextParcel(void);
uint8_t IRRIGATION_CTRL_IsParcelComplete(void);
uint8_t IRRIGATION_CTRL_GetCurrentParcelId(void);
uint32_t IRRIGATION_CTRL_GetRemainingTime(void);
void IRRIGATION_CTRL_AddToQueue(uint8_t parcel_id);
void IRRIGATION_CTRL_ClearQueue(void);

/* Mixing & Settling --------------------------------------------------------*/
void IRRIGATION_CTRL_StartMixing(void);
void IRRIGATION_CTRL_StopMixing(void);
uint8_t IRRIGATION_CTRL_IsMixingComplete(void);
void IRRIGATION_CTRL_StartSettling(void);
uint8_t IRRIGATION_CTRL_IsSettlingComplete(void);

/* Error Handling -----------------------------------------------------------*/
void IRRIGATION_CTRL_SetError(error_code_t error);
void IRRIGATION_CTRL_ClearError(error_code_t error);
void IRRIGATION_CTRL_ClearAllErrors(void);
error_code_t IRRIGATION_CTRL_GetLastError(void);
uint8_t IRRIGATION_CTRL_HasErrors(void);
const char *IRRIGATION_CTRL_GetErrorString(error_code_t error);
void IRRIGATION_CTRL_EmergencyStop(void);
uint8_t IRRIGATION_CTRL_AcknowledgeAlarm(void);
uint8_t IRRIGATION_CTRL_ResetAlarm(void);
uint8_t IRRIGATION_CTRL_CanResetAlarm(void);
uint8_t IRRIGATION_CTRL_IsAlarmAcknowledged(void);
const char *IRRIGATION_CTRL_GetAlarmDetailText(void);
const char *IRRIGATION_CTRL_GetAlarmActionText(void);

/* Statistics ---------------------------------------------------------------*/
uint32_t IRRIGATION_CTRL_GetTotalWateringTime(void);
uint32_t IRRIGATION_CTRL_GetTotalParcelCount(void);
uint32_t IRRIGATION_CTRL_GetCurrentRunTime(void);
void IRRIGATION_CTRL_ResetStatistics(void);

void IRRIGATION_CTRL_SetSchedule(uint8_t slot, irrigation_schedule_t *sched);
void IRRIGATION_CTRL_GetSchedule(uint8_t slot, irrigation_schedule_t *sched);
void IRRIGATION_CTRL_CheckSchedules(void);
void IRRIGATION_CTRL_StartProgram(uint8_t program_id);
void IRRIGATION_CTRL_CancelProgram(void);
uint8_t IRRIGATION_CTRL_GetActiveProgram(void);
program_state_t IRRIGATION_CTRL_GetProgramState(void);
void IRRIGATION_CTRL_GetProgram(uint8_t program_id, irrigation_program_t *program);
void IRRIGATION_CTRL_SetProgram(uint8_t program_id,
                                const irrigation_program_t *program);
void IRRIGATION_CTRL_LoadPrograms(void);
void IRRIGATION_CTRL_LoadRuntimeBackup(void);
void IRRIGATION_CTRL_MaintenanceTask(void);
uint8_t IRRIGATION_CTRL_RunSafetyChecks(void);
const char *IRRIGATION_CTRL_GetActiveAlarmText(void);
void IRRIGATION_CTRL_GetRuntimeBackup(irrigation_runtime_backup_t *backup);

/* Callbacks ----------------------------------------------------------------*/
void IRRIGATION_CTRL_OnStateChange(control_state_t old_state,
                                   control_state_t new_state);
void IRRIGATION_CTRL_OnPHAdjusted(float new_ph);
void IRRIGATION_CTRL_OnECAdjusted(float new_ec);
void IRRIGATION_CTRL_OnParcelComplete(uint8_t parcel_id);
void IRRIGATION_CTRL_OnError(error_code_t error);

#ifdef __cplusplus
}
#endif

#endif /* __IRRIGATION_CONTROL_H */
