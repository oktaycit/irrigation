/**
 ******************************************************************************
 * @file           : buzzer.c
 * @brief          : Buzzer Driver Implementation (TIM2 PWM)
 ******************************************************************************
 */

#include "main.h"
#include "buzzer.h"
#include <string.h>

extern TIM_HandleTypeDef htim2;

/* Buzzer State */
static uint16_t g_buzzer_freq = BUZZER_DEFAULT_FREQ;
static uint8_t g_buzzer_on = 0;
static uint8_t g_buzzer_available = 0;

/**
 * @brief  Initialize Buzzer
 */
void BUZZER_Init(void) {
    g_buzzer_freq = BUZZER_DEFAULT_FREQ;
    g_buzzer_on = 0;
    g_buzzer_available = 0;
    
    /* Start PWM with 0% duty cycle (off) */
    __HAL_TIM_SET_COMPARE(&htim2, BUZZER_TIM_CHANNEL, 0);
    if (HAL_TIM_PWM_Start(&htim2, BUZZER_TIM_CHANNEL) != HAL_OK) {
        return;
    }
    g_buzzer_available = 1;
}

/**
 * @brief  Turn buzzer ON
 */
void BUZZER_On(void) {
    if (!g_buzzer_available) return;
    uint32_t period = htim2.Init.Period;
    uint32_t duty_cycle = period / 2;  /* 50% duty cycle */
    
    BUZZER_SetFrequency(g_buzzer_freq);
    __HAL_TIM_SET_COMPARE(&htim2, BUZZER_TIM_CHANNEL, (uint32_t)duty_cycle);
    g_buzzer_on = 1;
}

/**
 * @brief  Turn buzzer OFF
 */
void BUZZER_Off(void) {
    if (!g_buzzer_available) return;
    __HAL_TIM_SET_COMPARE(&htim2, BUZZER_TIM_CHANNEL, 0);
    g_buzzer_on = 0;
}

/**
 * @brief  Toggle buzzer state
 */
void BUZZER_Toggle(void) {
    if (g_buzzer_on) {
        BUZZER_Off();
    } else {
        BUZZER_On();
    }
}

/**
 * @brief  Set buzzer frequency
 * @param  freq_hz: Frequency in Hz (500-5000)
 */
void BUZZER_SetFrequency(uint16_t freq_hz) {
    if (!g_buzzer_available) return;
    if (freq_hz < BUZZER_FREQ_MIN) freq_hz = BUZZER_FREQ_MIN;
    if (freq_hz > BUZZER_FREQ_MAX) freq_hz = BUZZER_FREQ_MAX;
    
    g_buzzer_freq = freq_hz;
    
    /* TIM2 runs at 2MHz (168MHz / 84 prescaler) */
    /* Period = 2MHz / freq - 1 */
    uint32_t period = (2000000U / freq_hz) - 1;
    __HAL_TIM_SET_AUTORELOAD(&htim2, period);
}

/**
 * @brief  Generate a beep for specified duration
 * @param  duration_ms: Duration in milliseconds
 */
void BUZZER_Beep(uint16_t duration_ms) {
    BUZZER_On();
    HAL_Delay(duration_ms);
    BUZZER_Off();
}

/**
 * @brief  Generate beep pattern
 * @param  pattern: Array of durations (ON, OFF, ON, OFF, ...)
 * @param  pattern_length: Number of elements in pattern
 */
void BUZZER_BeepPattern(const uint16_t *pattern, uint8_t pattern_length) {
    if (pattern == NULL || pattern_length == 0) return;
    
    for (uint8_t i = 0; i < pattern_length; i++) {
        if (i % 2 == 0) {
            BUZZER_On();
        } else {
            BUZZER_Off();
        }
        HAL_Delay(pattern[i]);
    }
    BUZZER_Off();
}

/**
 * @brief  Check if buzzer is currently on
 */
uint8_t BUZZER_IsOn(void) {
    return (uint8_t)(g_buzzer_available && g_buzzer_on);
}

/* Predefined patterns -------------------------------------------------------*/

void BUZZER_BeepShort(void) {
    BUZZER_Beep(100);
}

void BUZZER_BeepLong(void) {
    BUZZER_Beep(500);
}

void BUZZER_BeepDouble(void) {
    uint16_t pattern[] = {100, 100, 100, 200};  /* ON, OFF, ON, OFF */
    BUZZER_BeepPattern(pattern, 4);
}

void BUZZER_BeepError(void) {
    uint16_t pattern[] = {300, 100, 300, 100, 300, 300};  /* 3x long with pauses */
    BUZZER_BeepPattern(pattern, 6);
}

void BUZZER_BeepSuccess(void) {
    uint16_t pattern[] = {150, 50, 150, 200};  /* 2x short */
    BUZZER_BeepPattern(pattern, 4);
}

void BUZZER_BeepWarning(void) {
    uint16_t pattern[] = {400, 100, 150, 200};  /* Long + short */
    BUZZER_BeepPattern(pattern, 4);
}
