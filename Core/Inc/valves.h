/**
 ******************************************************************************
 * @file           : valves.h
 * @brief          : Vana Kontrol Driver (PCF8574 I2C Expander ile)
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
#include "pcf8574.h"

/* Vana Configuration -------------------------------------------------------*/
/* PCF8574 Configuration - ONLY for Parcel Valves */
#define VALVES_PCF_CHIP_PARCEL   0U     /* PCF8574 #0 - Parsel vanaları (1-8) */

/* Parsel Vanaları (1-8) - PCF8574 #0, P0-P7 */
#define PARCEL_VALVE_COUNT     8U
#define PARCEL_VALVE_1         1U       /* PCF8574 #0, P0 */
#define PARCEL_VALVE_2         2U       /* PCF8574 #0, P1 */
#define PARCEL_VALVE_3         3U       /* PCF8574 #0, P2 */
#define PARCEL_VALVE_4         4U       /* PCF8574 #0, P3 */
#define PARCEL_VALVE_5         5U       /* PCF8574 #0, P4 */
#define PARCEL_VALVE_6         6U       /* PCF8574 #0, P5 */
#define PARCEL_VALVE_7         7U       /* PCF8574 #0, P6 */
#define PARCEL_VALVE_8         8U       /* PCF8574 #0, P7 */

/* Dozaj Vanaları - Direct GPIO Control
 *
 * SPI2 is reserved for the XPT2046 touch controller on the F4VE TFT header
 * (PB13/PB14/PB15 + PB12 CS + PC5 IRQ). Keep the dosing outputs away from
 * those pins so touch and valves do not fight over the same GPIO block.
 */
#define DOSING_VALVE_COUNT     5U
#define DOSING_VALVE_ACID_ID   9U       /* PB6 - PWM controlled */
#define DOSING_VALVE_FERT_A_ID 10U      /* PB7 - PWM controlled */
#define DOSING_VALVE_FERT_B_ID 11U      /* PC0 - direct GPIO */
#define DOSING_VALVE_FERT_C_ID 12U      /* PC1 - direct GPIO */
#define DOSING_VALVE_FERT_D_ID 13U      /* PC2 - direct GPIO */
#define DOSING_PWM_DEFAULT_DUTY 100U
#define DOSING_PWM_MIN_FREQ_HZ 10U
#define DOSING_PWM_MAX_FREQ_HZ 50U
#define DOSING_ACID_PWM_DEFAULT_FREQ_HZ 25U
#define DOSING_FERT_PWM_DEFAULT_FREQ_HZ 25U
#define DOSING_HW_PWM_FREQ_HZ 25U
#define DOSING_PWM_DEFAULT_MIN_DUTY 5U
#define DOSING_PWM_DEFAULT_MAX_DUTY 100U
#define DOSING_PWM_DEFAULT_RAMP_STEP 5U

/* Toplam Vana Sayısı */
#define TOTAL_VALVE_COUNT      (PARCEL_VALVE_COUNT + DOSING_VALVE_COUNT)
#define VALVE_COUNT            TOTAL_VALVE_COUNT  /* Alias for compatibility */

/* Dozaj Vanaları GPIO Pins (Direct MCU Control) */
#define DOSING_ACID_PIN        GPIO_PIN_6
#define DOSING_ACID_PORT       GPIOB
#define DOSING_FERT_A_PIN      GPIO_PIN_7
#define DOSING_FERT_A_PORT     GPIOB
#define DOSING_FERT_B_PIN      GPIO_PIN_0
#define DOSING_FERT_B_PORT     GPIOC
#define DOSING_FERT_C_PIN      GPIO_PIN_1
#define DOSING_FERT_C_PORT     GPIOC
#define DOSING_FERT_D_PIN      GPIO_PIN_2
#define DOSING_FERT_D_PORT     GPIOC

/* Vana Tipi ---------------------------------------------------------------*/
typedef enum {
  VALVE_TYPE_PARCEL = 0,     /* Parsel sulama vanası */
  VALVE_TYPE_DOSING_ACID,    /* Asit dozaj vanası */
  VALVE_TYPE_DOSING_FERT,    /* Gübre dozaj vanası */
  VALVE_TYPE_MIXER,          /* Karıştırıcı */
  VALVE_TYPE_DRAIN           /* Tahliye vanası */
} valve_type_t;

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

/* Vana Fault Flags ---------------------------------------------------------*/
typedef enum {
  VALVE_FAULT_NONE = 0x00U,
  VALVE_FAULT_INVALID_ID = 0x01U,
  VALVE_FAULT_DRIVER = 0x02U,
  VALVE_FAULT_DISABLED = 0x04U,
  VALVE_FAULT_PARCEL_INTERLOCK = 0x08U,
  VALVE_FAULT_DOSING_INTERLOCK = 0x10U,
  VALVE_FAULT_STATE_MISMATCH = 0x20U,
  VALVE_FAULT_TIMEOUT = 0x40U,
  VALVE_FAULT_EMERGENCY_STOP = 0x80U
} valve_fault_flag_t;

/* Vana Structure -----------------------------------------------------------*/
typedef struct {
  uint8_t id;
  valve_state_t state;
  valve_mode_t mode;
  valve_type_t type;        /* Vana tipi (parsel/dozaj) */
  uint8_t is_active;
  uint32_t on_time_ms;
  uint32_t off_time_ms;
  uint32_t total_on_time;   /* Toplam açık kalma süresi (saniye) */
  uint32_t cycle_count;     /* Toplam açma/kapama sayısı */
  uint32_t last_action_time;
  uint8_t fault_flags;
  uint8_t interlock_source_id;
} valve_t;

typedef struct {
  uint8_t frequency_hz;
  uint8_t min_duty_percent;
  uint8_t max_duty_percent;
  uint8_t target_duty_percent;
  uint8_t applied_duty_percent;
  uint8_t ramp_step_percent;
  uint8_t enabled;
  uint8_t output_state;
} dosing_channel_status_t;

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
uint8_t VALVES_RequestOpen(uint8_t valve_id);
uint8_t VALVES_RequestClose(uint8_t valve_id);
void VALVES_Open(uint8_t valve_id);
void VALVES_Close(uint8_t valve_id);
void VALVES_Toggle(uint8_t valve_id);
uint8_t VALVES_GetState(uint8_t valve_id);
uint8_t VALVES_GetFaultFlags(uint8_t valve_id);
void VALVES_ClearFaultFlags(uint8_t valve_id);
uint8_t VALVES_GetInterlockSource(uint8_t valve_id);
void VALVES_SetMode(uint8_t valve_id, valve_mode_t mode);
valve_mode_t VALVES_GetMode(uint8_t valve_id);
void VALVES_SetTiming(uint8_t valve_id, uint32_t on_ms, uint32_t off_ms);
void VALVES_Update(void);
void VALVES_TimerTick1ms(void);

/* Tüm Vanaları Kontrol -----------------------------------------------------*/
void VALVES_OpenAll(void);
void VALVES_CloseAll(void);
void VALVES_StopAll(void);
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

/* Dozaj Vana Fonksiyonları -------------------------------------------------*/
uint8_t VALVES_IsDosingValve(uint8_t valve_id);
void VALVES_OpenDosing(uint8_t dosing_idx);
void VALVES_CloseDosing(uint8_t dosing_idx);
void VALVES_ConfigureDosingChannel(uint8_t valve_id, uint8_t frequency_hz,
                                   uint8_t min_duty_percent,
                                   uint8_t max_duty_percent,
                                   uint8_t ramp_step_percent);
void VALVES_SetDosingDuty(uint8_t valve_id, uint8_t duty_percent);
uint8_t VALVES_GetDosingDuty(uint8_t valve_id);
uint8_t VALVES_GetDosingAppliedDuty(uint8_t valve_id);
void VALVES_SetDosingFrequency(uint8_t valve_id, uint8_t frequency_hz);
uint8_t VALVES_GetDosingFrequency(uint8_t valve_id);
uint8_t VALVES_IsDosingEnabled(uint8_t valve_id);
uint8_t VALVES_GetDosingOutputState(uint8_t valve_id);
void VALVES_GetDosingStatus(uint8_t valve_id, dosing_channel_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* __VALVES_H */
