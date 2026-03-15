/**
 ******************************************************************************
 * @file           : valves.h
 * @brief          : 8 Kanal Vana Kontrol Driver
 ******************************************************************************
 */

#ifndef __VALVES_H
#define __VALVES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

/* Vana Configuration -------------------------------------------------------*/
#define VALVE_COUNT 8U

/* Vana GPIO Pins -----------------------------------------------------------*/
#define VALVE_1_PIN GPIO_PIN_0
#define VALVE_2_PIN GPIO_PIN_1
#define VALVE_3_PIN GPIO_PIN_2
#define VALVE_4_PIN GPIO_PIN_3
#define VALVE_5_PIN GPIO_PIN_4
#define VALVE_6_PIN GPIO_PIN_5
#define VALVE_7_PIN GPIO_PIN_6
#define VALVE_8_PIN GPIO_PIN_7
#define VALVE_PORT GPIOB

/* Vana Status --------------------------------------------------------------*/
#define VALVE_STATUS_OFF 0U
#define VALVE_STATUS_ON 1U
#define VALVE_STATUS_ERROR 2U

/* Vana Control Modes -------------------------------------------------------*/
typedef enum {
  VALVE_MODE_MANUAL = 0,
  VALVE_MODE_AUTO,
  VALVE_MODE_SCHEDULED,
  VALVE_MODE_DISABLED
} valve_mode_t;

/* Vana State ---------------------------------------------------------------*/
typedef enum {
  VALVE_STATE_CLOSED = 0,
  VALVE_STATE_OPEN,
  VALVE_STATE_OPENING,
  VALVE_STATE_CLOSING,
  VALVE_STATE_ERROR
} valve_state_t;

/* Vana Structure -----------------------------------------------------------*/
typedef struct {
  uint8_t id;
  valve_state_t state;
  valve_mode_t mode;
  uint8_t is_active;
  uint32_t on_time_ms;
  uint32_t off_time_ms;
  uint32_t total_on_time; /* Toplam açık kalma süresi (sanitye) */
  uint32_t cycle_count;   /* Toplam açma/kapama sayısı */
  uint32_t last_action_time;
} valve_t;

/* Parsel Yapısı ------------------------------------------------------------*/
typedef struct {
  uint8_t parcel_id;
  uint8_t valve_id;
  char name[16];
  uint32_t irrigation_duration_sec; /* Sulama süresi (saniye) */
  uint32_t wait_duration_sec;       /* Bekleme süresi (saniye) */
  uint8_t enabled;
  uint8_t is_watering;
} parcel_t;

/* Sulama Modları -----------------------------------------------------------*/
typedef enum {
  IRRIGATION_MODE_OFF = 0,
  IRRIGATION_MODE_SINGLE,   /* Tek parsel sulama */
  IRRIGATION_MODE_SEQUENCE, /* Sıralı sulama (tüm parseller) */
  IRRIGATION_MODE_CUSTOM    /* Özel program */
} irrigation_mode_t;

/* Sulama Durumu ------------------------------------------------------------*/
typedef struct {
  irrigation_mode_t mode;
  uint8_t is_running;
  uint8_t current_parcel;
  uint8_t total_parcels;
  uint32_t elapsed_time;
  uint32_t remaining_time;
  uint8_t completed_parcels;
} irrigation_status_t;

/* Vana Fonksiyonları -------------------------------------------------------*/
void VALVES_Init(void);
void VALVES_Open(uint8_t valve_id);
void VALVES_Close(uint8_t valve_id);
void VALVES_Toggle(uint8_t valve_id);
uint8_t VALVES_GetState(uint8_t valve_id);
void VALVES_SetMode(uint8_t valve_id, valve_mode_t mode);
valve_mode_t VALVES_GetMode(uint8_t valve_id);
void VALVES_SetTiming(uint8_t valve_id, uint32_t on_ms, uint32_t off_ms);
void VALVES_Update(void);

/* Tüm Vanaları Kontrol -----------------------------------------------------*/
void VALVES_OpenAll(void);
void VALVES_CloseAll(void);
uint16_t VALVES_GetActiveMask(void);
void VALVES_SetActiveMask(uint16_t mask);

/* Parsel Fonksiyonları -----------------------------------------------------*/
void PARCELS_Init(void);
void PARCELS_SetDuration(uint8_t parcel_id, uint32_t duration_sec);
uint32_t PARCELS_GetDuration(uint8_t parcel_id);
void PARCELS_SetEnabled(uint8_t parcel_id, uint8_t enabled);
uint8_t PARCELS_IsEnabled(uint8_t parcel_id);
void PARCELS_SetName(uint8_t parcel_id, const char *name);
const char *PARCELS_GetName(uint8_t parcel_id);

/* Sulama Kontrol Fonksiyonları ---------------------------------------------*/
void IRRIGATION_Start(irrigation_mode_t mode, uint8_t parcel_id);
void IRRIGATION_Stop(void);
void IRRIGATION_Pause(void);
void IRRIGATION_Resume(void);
void IRRIGATION_Update(void);
irrigation_status_t IRRIGATION_GetStatus(void);
uint8_t IRRIGATION_IsRunning(void);
uint8_t IRRIGATION_GetCurrentParcel(void);
uint32_t IRRIGATION_GetRemainingTime(void);

/* Manuel Kontrol -----------------------------------------------------------*/
void VALVES_ManualOpen(uint8_t valve_id);
void VALVES_ManualClose(uint8_t valve_id);
uint8_t VALVES_IsManualMode(uint8_t valve_id);

/* Güvenlik Fonksiyonları ---------------------------------------------------*/
void VALVES_EmergencyStop(void);
void VALVES_CheckErrors(void);
uint8_t VALVES_GetErrorMask(void);
void VALVES_ClearErrors(void);

/* İstatistikler ------------------------------------------------------------*/
uint32_t VALVES_GetTotalOnTime(uint8_t valve_id);
uint32_t VALVES_GetCycleCount(uint8_t valve_id);
void VALVES_ResetStatistics(uint8_t valve_id);

#ifdef __cplusplus
}
#endif

#endif /* __VALVES_H */