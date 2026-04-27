/**
 ******************************************************************************
 * @file           : low_power.h
 * @brief          : Low-Power Modes Driver Header (Sleep/Stop)
 ******************************************************************************
 */

#ifndef __LOW_POWER_H
#define __LOW_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Low-Power Configuration ---------------------------------------------------*/
#define LOW_POWER_ENABLED         1U
#define STOP_MODE_ENABLED         1U
#define SLEEP_MODE_ENABLED        1U

/* Wakeup Sources -----------------------------------------------------------*/
#define WAKEUP_SOURCE_TOUCH       (1U << 0)
#define WAKEUP_SOURCE_RTC         (1U << 1)
#define WAKEUP_SOURCE_BUTTON      (1U << 2)
#define WAKEUP_SOURCE_UART        (1U << 3)

/* Low-Power Modes ----------------------------------------------------------*/
typedef enum {
    LOW_POWER_RUN = 0,        /* Normal run mode */
    LOW_POWER_SLEEP,          /* Sleep mode - CPU stopped, peripherals running */
    LOW_POWER_STOP            /* Stop mode - CPU and clocks stopped */
} low_power_mode_t;

/* Low-Power Functions ------------------------------------------------------*/
void LOW_POWER_Init(void);
void LOW_POWER_EnterMode(low_power_mode_t mode);
void LOW_POWER_EnterSleep(uint32_t wakeup_sources);
void LOW_POWER_EnterStop(uint32_t wakeup_sources);
void LOW_POWER_ExitStop(void);
low_power_mode_t LOW_POWER_GetCurrentMode(void);

/* Power Management ---------------------------------------------------------*/
void LOW_POWER_DisablePeripherals(void);
void LOW_POWER_EnablePeripherals(void);
void LOW_POWER_SaveContext(void);
void LOW_POWER_RestoreContext(void);

/* Wakeup Handling ----------------------------------------------------------*/
uint32_t LOW_POWER_GetWakeupSource(void);
void LOW_POWER_ClearWakeupFlags(void);
void LOW_POWER_ConfigWakeupPin(void);

/* Utility ------------------------------------------------------------------*/
uint32_t LOW_POWER_GetSleepDuration(void);
void LOW_POWER_SetAutoSleepTimeout(uint32_t timeout_ms);
void LOW_POWER_CheckAutoSleep(void);
void LOW_POWER_UpdateActivity(void);
void LOW_POWER_OnWakeup(uint32_t wakeup_source);

#ifdef __cplusplus
}
#endif

#endif /* __LOW_POWER_H */
