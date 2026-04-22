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
extern UART_HandleTypeDef huart1;
extern I2C_HandleTypeDef hi2c1;
extern SPI_HandleTypeDef hspi2;
extern ADC_HandleTypeDef hadc1;
extern volatile uint16_t g_adc_dma_buffer[ADC_DMA_BUFFER_SIZE];

static low_power_mode_t g_current_mode = LOW_POWER_RUN;
static uint32_t g_wakeup_sources = 0;
static uint32_t g_auto_sleep_timeout = 0;
static uint32_t g_last_activity_time = 0;
static uint8_t g_context_saved = 0;

/* Register backup values for Stop mode */
static struct {
    uint32_t sysclk;
    uint32_t hclk;
    uint32_t pclk1;
    uint32_t pclk2;
    uint32_t flash_latency;
} g_register_backup;

/**
 * @brief  Initialize Low-Power functionality
 */
void LOW_POWER_Init(void) {
    g_current_mode = LOW_POWER_RUN;
    g_wakeup_sources = 0;
    g_auto_sleep_timeout = 300000;  /* 5 minutes default */
    g_last_activity_time = HAL_GetTick();
    
    /* Configure wakeup pins */
    LOW_POWER_ConfigWakeupPin();
    
    /* Enable Power Clock */
    __HAL_RCC_PWR_CLK_ENABLE();
    
    /* Enable Wakeup pin */
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);  /* PA0 */
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN2);  /* PA2 */
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
    /* Update last activity time */
    g_last_activity_time = HAL_GetTick();
    
    /* Configure wakeup sources */
    g_wakeup_sources = wakeup_sources;
    
    /* Enter Sleep mode */
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    
    /* Exit from Sleep mode - restore clocks */
    SystemClock_Config();
}

/**
 * @brief  Enter Stop mode
 */
void LOW_POWER_EnterStop(uint32_t wakeup_sources) {
    /* Save context */
    LOW_POWER_SaveContext();
    g_context_saved = 1;
    
    /* Disable peripherals to save power */
    LOW_POWER_DisablePeripherals();
    
    /* Configure wakeup sources */
    g_wakeup_sources = wakeup_sources;
    
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
    
    /* Clear wakeup flags */
    LOW_POWER_ClearWakeupFlags();
    
    g_current_mode = LOW_POWER_RUN;
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
    HAL_TIM_Base_Stop(&htim3);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    
    /* Disable UART */
    HAL_UART_Suspend_IT(&huart1);
    
    /* Disable I2C */
    HAL_I2C_MspDeInit(&hi2c1);
    
    /* Disable SPI */
    HAL_SPI_Suspend(&hspi2);
    
    /* Reduce clock speed before entering STOP */
    __HAL_RCC_HSI_DISABLE();
}

/**
 * @brief  Enable peripherals after exiting Stop mode
 */
void LOW_POWER_EnablePeripherals(void) {
    /* Re-enable I2C */
    HAL_I2C_MspInit(&hi2c1);
    
    /* Restart timers */
    HAL_TIM_Base_Start(&htim3);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    
    /* Restart ADC DMA */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&g_adc_dma_buffer, ADC_DMA_BUFFER_SIZE);
    
    /* Restart UART */
    HAL_UART_Resume_IT(&huart1);
    
    /* Restart SPI */
    HAL_SPI_Resume(&hspi2);
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
    if (__HAL_RTC_ALARM_GET_FLAG(&hrtc, RTC_FLAG_ALRAF) != RESET) {
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
    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRBF);
}

/**
 * @brief  Configure wakeup pin (PA0 - WKUP)
 */
void LOW_POWER_ConfigWakeupPin(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    /* Enable GPIOA clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    /* Configure PA0 as wakeup input */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* Configure EXTI for touch IRQ (PC5) */
    /* Already configured in MX_GPIO_Init */
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
