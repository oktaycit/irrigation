/**
 ******************************************************************************
 * @file           : buzzer.h
 * @brief          : Buzzer Driver Header (TIM2 PWM)
 ******************************************************************************
 */

#ifndef __BUZZER_H
#define __BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Buzzer Configuration ------------------------------------------------------*/
#define BUZZER_TIM            TIM2
#define BUZZER_TIM_CHANNEL    TIM_CHANNEL_1
#define BUZZER_FREQ_MIN       500U      /* 500 Hz minimum */
#define BUZZER_FREQ_MAX       5000U     /* 5 kHz maximum */
#define BUZZER_DEFAULT_FREQ   2000U     /* 2 kHz default */

/* Buzzer Control ------------------------------------------------------------*/
void BUZZER_Init(void);
void BUZZER_On(void);
void BUZZER_Off(void);
void BUZZER_Toggle(void);
void BUZZER_SetFrequency(uint16_t freq_hz);
void BUZZER_Beep(uint16_t duration_ms);
void BUZZER_BeepPattern(const uint16_t *pattern, uint8_t pattern_length);
uint8_t BUZZER_IsOn(void);

/* Predefined beep patterns --------------------------------------------------*/
void BUZZER_BeepShort(void);     /* 100ms beep */
void BUZZER_BeepLong(void);      /* 500ms beep */
void BUZZER_BeepDouble(void);    /* 2x short beep */
void BUZZER_BeepError(void);     /* Error pattern: 3x long */
void BUZZER_BeepSuccess(void);   /* Success pattern: 2x short */
void BUZZER_BeepWarning(void);   /* Warning: long + short */

#ifdef __cplusplus
}
#endif

#endif /* __BUZZER_H */
