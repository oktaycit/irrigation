/**
 ******************************************************************************
 * @file           : low_power.c
 * @brief          : Low-Power Modes Driver Implementation (Sleep/Stop)
 ******************************************************************************
 */

#include "main.h"
#include "low_power.h"
#include <string.h>

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern UART_HandleTypeDef huart1;
extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi1;
extern SPI_HandleTypeDef hspi2;
extern ADC_HandleTypeDef hadc1;
extern volatile uint16_t g_adc_dma_buffer[ADC_DMA_BUFFER_SIZE];

static low_power_mode_t g_current_mode = LOW_POWER_RUN;
static uint32_t g_wakeup_sources = 0;
static uint32_t g_auto_sleep_timeout = 0;
static uint32_t g_last_activity_time = 0;
static uint8_t g_context_saved = 0;

/* Register backup values for Stop mode */
/**
 * @brief  Initialize Low-Power functionality
 */
void LOW_POWER_Init(void) {
    g_current_mode = LOW_POWER_RUN;
    g_wakeup_sources = 0;
    g_auto_sleep_timeout = 300000;  /* 5 minutes default */
    g_last_activity_time = HAL_GetTick();
    
    /* Enable Power Clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /*
     * PA0 is used as ADC1_IN0 for pH sensing on this board, so we avoid
     * enabling/configuring WKUP1 here. Touch EXTI and RTC alarm are enough for
     * the sleep/stop paths used by the application.
     */
    LOW_POWER_ConfigWakeupPin();
}

/**
 * @brief  Enter specified low-power mode
 */
void LOW_POWER_EnterMode(low_power_mode_t mode) {
    switch (mode) {
        case LOW_POWER_SLEEP:
            LOW_POWER_EnterSleep(WAKEUP_SOURCE_TOUCH | WAKEUP_SOURCE_UART);
            break;
            
        case LOW_POWER_STOP:
            LOW_POWER_EnterStop(WAKEUP_SOURCE_TOUCH | WAKEUP_SOURCE_RTC);
            break;
            
        default:
            /* LOW_POWER_RUN - do nothing */
            break;
    }
}

/**
 * @brief  Enter Sleep mode
 */
void LOW_POWER_EnterSleep(uint32_t wakeup_sources) {
    uint32_t wakeup_source;

    /* Configure wakeup sources */
    g_wakeup_sources = wakeup_sources;
    g_current_mode = LOW_POWER_SLEEP;
    
    /* Enter Sleep mode */
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

    wakeup_source = LOW_POWER_GetWakeupSource();
    g_current_mode = LOW_POWER_RUN;
    LOW_POWER_OnWakeup(wakeup_source);
}

/**
 * @brief  Enter Stop mode
 */
void LOW_POWER_EnterStop(uint32_t wakeup_sources) {
    uint32_t wakeup_source;

    /* Save context */
    LOW_POWER_SaveContext();
    g_context_saved = 1;
    
    /* Disable peripherals to save power */
    LOW_POWER_DisablePeripherals();
    
    /* Configure wakeup sources */
    g_wakeup_sources = wakeup_sources;
    g_current_mode = LOW_POWER_STOP;
    
    /* Configure RTC wakeup if enabled */
    if (wakeup_sources & WAKEUP_SOURCE_RTC) {
        /* RTC alarm already configured */
    }
    
    /* Enter Stop mode */
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    
    /* Exit from Stop mode - reconfigure system clock */
    SystemClock_Config();
    
    /* Restore peripherals */
    LOW_POWER_EnablePeripherals();

    wakeup_source = LOW_POWER_GetWakeupSource();
    
    /* Clear wakeup flags */
    LOW_POWER_ClearWakeupFlags();
    
    g_current_mode = LOW_POWER_RUN;
    LOW_POWER_OnWakeup(wakeup_source);
}

/**
 * @brief  Exit Stop mode and restore operation
 */
void LOW_POWER_ExitStop(void) {
    if (g_context_saved) {
        LOW_POWER_RestoreContext();
        g_context_saved = 0;
    }
}

/**
 * @brief  Get current low-power mode
 */
low_power_mode_t LOW_POWER_GetCurrentMode(void) {
    return g_current_mode;
}

/**
 * @brief  Disable peripherals before entering Stop mode
 */
void LOW_POWER_DisablePeripherals(void) {
    /* Stop ADC DMA */
    HAL_ADC_Stop_DMA(&hadc1);
    
    /* Stop timers */
    HAL_TIM_Base_Stop_IT(&htim3);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2);
    
    /* Stop serial DMA before clocks are gated in Stop mode. */
    HAL_UART_DMAStop(&huart1);
    HAL_UART_DeInit(&huart1);
    
    /* Disable synchronous peripherals cleanly. Init structs are retained. */
    HAL_I2C_DeInit(&hi2c1);
    
    HAL_SPI_DeInit(&hspi1);
    HAL_SPI_DeInit(&hspi2);
}

/**
 * @brief  Enable peripherals after exiting Stop mode
 */
void LOW_POWER_EnablePeripherals(void) {
    /* Re-enable I2C */
    HAL_I2C_Init(&hi2c1);
    
    /* Restart SPI */
    HAL_SPI_Init(&hspi1);
    HAL_SPI_Init(&hspi2);

    /* Restart UART */
    HAL_UART_Init(&huart1);
    
    /* Restart timers */
    HAL_TIM_Base_Start_IT(&htim3);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
    
    /* Restart ADC DMA */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&g_adc_dma_buffer, ADC_DMA_BUFFER_SIZE);
}

/**
 * @brief  Save context before entering Stop mode
 */
void LOW_POWER_SaveContext(void) {
    /* Backup register values if needed */
    /* For now, system clock config is sufficient */
}

/**
 * @brief  Restore context after exiting Stop mode
 */
void LOW_POWER_RestoreContext(void) {
    /* Restore system clock */
    SystemClock_Config();
    
    /* Re-initialize peripherals */
    /* Most peripherals will be re-initialized by their drivers */
}

/**
 * @brief  Get wakeup source flags
 */
uint32_t LOW_POWER_GetWakeupSource(void) {
    uint32_t sources = 0;
    
    /* Check PWR wakeup flags */
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_WU) != RESET) {
        sources |= WAKEUP_SOURCE_BUTTON;
    }
    
    /* Check RTC wakeup/alarm flags */
    if ((RTC->ISR & (RTC_ISR_ALRAF | RTC_ISR_ALRBF)) != 0U) {
        sources |= WAKEUP_SOURCE_RTC;
    }
    
    /* Check EXTI for touch */
    if (HAL_GPIO_ReadPin(TOUCH_IRQ_PORT, TOUCH_IRQ_PIN) == GPIO_PIN_RESET) {
        sources |= WAKEUP_SOURCE_TOUCH;
    }
    
    return sources;
}

/**
 * @brief  Clear all wakeup flags
 */
void LOW_POWER_ClearWakeupFlags(void) {
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    RTC->WPR = 0xCAU;
    RTC->WPR = 0x53U;
    RTC->ISR &= ~(RTC_ISR_ALRAF | RTC_ISR_ALRBF);
    RTC->WPR = 0xFFU;
    EXTI->PR = EXTI_PR_PR17;
}

/**
 * @brief  Configure wakeup pin (PA0 - WKUP)
 */
void LOW_POWER_ConfigWakeupPin(void) {
    /*
     * Touch EXTI is configured in MX_GPIO_Init and RTC alarm EXTI is configured
     * by RTC_Init. PA0 is an ADC input, so no wakeup GPIO is configured here.
     */
}

/**
 * @brief  Get recommended sleep duration based on system state
 */
uint32_t LOW_POWER_GetSleepDuration(void) {
    /* Check if irrigation is running */
    if (IRRIGATION_CTRL_IsRunning()) {
        return 0;  /* Don't sleep during irrigation */
    }
    
    /* Check if sensors need frequent reading */
    if (SENSORS_INTERFACE_MODBUS == 0) {
        return 1000;  /* 1 second max for ADC mode */
    }
    
    return 60000;  /* 1 minute for Modbus mode */
}

/**
 * @brief  Set auto-sleep timeout
 */
void LOW_POWER_SetAutoSleepTimeout(uint32_t timeout_ms) {
    g_auto_sleep_timeout = timeout_ms;
}

/**
 * @brief  Check if system should enter auto-sleep
 */
void LOW_POWER_CheckAutoSleep(void) {
    uint32_t idle_time = HAL_GetTick() - g_last_activity_time;
    
    if (idle_time >= g_auto_sleep_timeout) {
        /* Check if it's safe to sleep */
        if (!IRRIGATION_CTRL_IsRunning() && 
            !IRRIGATION_CTRL_HasErrors()) {
            
            /* Enter sleep mode */
            LOW_POWER_EnterSleep(WAKEUP_SOURCE_TOUCH);
        }
    }
}

/**
 * @brief  Update last activity time (call on user interaction)
 */
void LOW_POWER_UpdateActivity(void) {
    g_last_activity_time = HAL_GetTick();
    
    /* Exit low-power mode if active */
    if (g_current_mode != LOW_POWER_RUN) {
        LOW_POWER_ExitStop();
    }
}

/* Weak definition - can be overridden in main */
__weak void LOW_POWER_OnWakeup(uint32_t wakeup_source) {
    (void)wakeup_source;
    /* User can implement custom wakeup handling */
}
