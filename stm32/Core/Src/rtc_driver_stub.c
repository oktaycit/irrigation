/**
 ******************************************************************************
 * @file           : rtc_driver_stub.c
 * @brief          : Direct-register RTC driver using STM32 backup domain
 ******************************************************************************
 */

#include "rtc_driver.h"
#include "main.h"
#include <string.h>

#define RTC_BKP_MAGIC 0x32F2A55AU
#define RTC_INIT_TIMEOUT_MS 5000U

static uint8_t g_rtc_initialized = 0U;
static rtc_alarm_t g_alarms[RTC_ALARM_COUNT] = {0};

static uint8_t RTC_ToBCD(uint8_t value);
static uint8_t RTC_FromBCD(uint32_t value);
static uint8_t RTC_WaitForFlag(volatile uint32_t *reg, uint32_t mask,
                               uint32_t expected_state, uint32_t timeout_ms);
static void RTC_DisableWriteProtection(void);
static void RTC_EnableWriteProtection(void);
static uint8_t RTC_EnterInitMode(void);
static void RTC_ExitInitMode(void);
static void RTC_EnsureClockConfigured(void);
static uint32_t RTC_PackTime(const rtc_time_t *time);
static uint32_t RTC_PackDate(const rtc_date_t *date);
static void RTC_UnpackTime(uint32_t tr, rtc_time_t *time);
static void RTC_UnpackDate(uint32_t dr, rtc_date_t *date);
static void RTC_ConfigureExtiAlarm(void);
static void RTC_ClearAlarmFlags(uint8_t alarm_id);

uint8_t RTC_Init(void) {
  RTC_EnsureClockConfigured();
  memset(g_alarms, 0, sizeof(g_alarms));
  for (uint8_t i = 0U; i < RTC_ALARM_COUNT; i++) {
    g_alarms[i].id = i + 1U;
  }

  RTC->CR |= RTC_CR_BYPSHAD;
  RTC_ConfigureExtiAlarm();
  g_rtc_initialized = 1U;
  return 1U;
}

uint8_t RTC_IsInitialized(void) { return g_rtc_initialized; }

void RTC_GetTime(rtc_time_t *time) {
  uint32_t tr;
  uint32_t dr;

  if (time == NULL || g_rtc_initialized == 0U) return;

  tr = RTC->TR;
  dr = RTC->DR;
  (void)dr;
  RTC_UnpackTime(tr, time);
}

void RTC_SetTime(const rtc_time_t *time) {
  uint32_t current_dr;

  if (time == NULL || g_rtc_initialized == 0U) return;

  RTC_DisableWriteProtection();
  if (RTC_EnterInitMode() == 0U) {
    RTC_EnableWriteProtection();
    return;
  }

  current_dr = RTC->DR;
  RTC->TR = RTC_PackTime(time);
  RTC->DR = current_dr;
  RTC_ExitInitMode();
  RTC_EnableWriteProtection();
}

void RTC_GetDate(rtc_date_t *date) {
  uint32_t tr;
  uint32_t dr;

  if (date == NULL || g_rtc_initialized == 0U) return;

  tr = RTC->TR;
  dr = RTC->DR;
  (void)tr;
  RTC_UnpackDate(dr, date);
}

void RTC_SetDate(const rtc_date_t *date) {
  uint32_t current_tr;

  if (date == NULL || g_rtc_initialized == 0U) return;

  RTC_DisableWriteProtection();
  if (RTC_EnterInitMode() == 0U) {
    RTC_EnableWriteProtection();
    return;
  }

  current_tr = RTC->TR;
  RTC->TR = current_tr;
  RTC->DR = RTC_PackDate(date);
  RTC_ExitInitMode();
  RTC_EnableWriteProtection();
}

uint8_t RTC_SetAlarm(uint8_t alarm_id, const rtc_time_t *time, uint8_t daily_repeat) {
  uint32_t alarm_value;
  volatile uint32_t *alarm_reg;
  uint32_t enable_mask;
  uint32_t writable_mask;

  if (alarm_id == 0U || alarm_id > RTC_ALARM_COUNT || time == NULL ||
      g_rtc_initialized == 0U) {
    return 0U;
  }

  alarm_reg = (alarm_id == 1U) ? &RTC->ALRMAR : &RTC->ALRMBR;
  enable_mask = (alarm_id == 1U) ? RTC_CR_ALRAE : RTC_CR_ALRBE;
  writable_mask = (alarm_id == 1U) ? RTC_ISR_ALRAWF : RTC_ISR_ALRBWF;

  alarm_value = RTC_PackTime(time) | RTC_ALRMAR_MSK1;
  if (daily_repeat != 0U) {
    alarm_value |= RTC_ALRMAR_MSK4;
  }

  RTC_DisableWriteProtection();
  RTC->CR &= ~enable_mask;
  if (RTC_WaitForFlag(&RTC->ISR, writable_mask, writable_mask, 100U) == 0U) {
    RTC_EnableWriteProtection();
    return 0U;
  }

  *alarm_reg = alarm_value;
  RTC_EnableWriteProtection();

  g_alarms[alarm_id - 1U].time = *time;
  g_alarms[alarm_id - 1U].daily_repeat = (daily_repeat != 0U) ? 1U : 0U;
  return 1U;
}

uint8_t RTC_EnableAlarm(uint8_t alarm_id) {
  uint32_t enable_mask;
  uint32_t interrupt_mask;

  if (alarm_id == 0U || alarm_id > RTC_ALARM_COUNT || g_rtc_initialized == 0U) {
    return 0U;
  }

  enable_mask = (alarm_id == 1U) ? RTC_CR_ALRAE : RTC_CR_ALRBE;
  interrupt_mask = (alarm_id == 1U) ? RTC_CR_ALRAIE : RTC_CR_ALRBIE;

  RTC_DisableWriteProtection();
  RTC_ClearAlarmFlags(alarm_id);
  RTC->CR |= enable_mask | interrupt_mask;
  RTC_EnableWriteProtection();

  g_alarms[alarm_id - 1U].enabled = 1U;
  return 1U;
}

uint8_t RTC_DisableAlarm(uint8_t alarm_id) {
  uint32_t enable_mask;
  uint32_t interrupt_mask;

  if (alarm_id == 0U || alarm_id > RTC_ALARM_COUNT || g_rtc_initialized == 0U) {
    return 0U;
  }

  enable_mask = (alarm_id == 1U) ? RTC_CR_ALRAE : RTC_CR_ALRBE;
  interrupt_mask = (alarm_id == 1U) ? RTC_CR_ALRAIE : RTC_CR_ALRBIE;

  RTC_DisableWriteProtection();
  RTC->CR &= ~(enable_mask | interrupt_mask);
  RTC_ClearAlarmFlags(alarm_id);
  RTC_EnableWriteProtection();

  g_alarms[alarm_id - 1U].enabled = 0U;
  return 1U;
}

void RTC_ClearAlarm(uint8_t alarm_id) {
  if (g_rtc_initialized == 0U) return;

  RTC_DisableWriteProtection();
  RTC_ClearAlarmFlags(alarm_id);
  RTC_EnableWriteProtection();
}

uint8_t RTC_GetAlarmState(uint8_t alarm_id) {
  if (alarm_id == 0U || alarm_id > RTC_ALARM_COUNT) {
    return 0U;
  }

  return g_alarms[alarm_id - 1U].enabled;
}

uint32_t RTC_GetTimestamp(void) {
  rtc_date_t date = {0};
  rtc_time_t time = {0};

  RTC_GetDate(&date);
  RTC_GetTime(&time);
  return RTC_DateTimeToTimestamp(&date, &time);
}

void RTC_TimestampToDateTime(uint32_t timestamp, rtc_date_t *date, rtc_time_t *time) {
  uint32_t days = timestamp / 86400UL;
  uint32_t remaining = timestamp % 86400UL;

  if (date == NULL || time == NULL) return;

  time->hours = (uint8_t)(remaining / 3600UL);
  time->minutes = (uint8_t)((remaining % 3600UL) / 60UL);
  time->seconds = (uint8_t)(remaining % 60UL);

  date->year = 0U;
  date->month = 1U;
  date->date = 1U;

  while (days >= (uint32_t)(RTC_IsLeapYear(date->year) != 0U ? 366U : 365U)) {
    days -= (uint32_t)(RTC_IsLeapYear(date->year) != 0U ? 366U : 365U);
    date->year++;
  }

  while (days >= (uint32_t)RTC_GetDaysInMonth(date->month, date->year)) {
    days -= RTC_GetDaysInMonth(date->month, date->year);
    date->month++;
  }

  date->date = (uint8_t)(date->date + days);
  date->weekday = (uint8_t)(((timestamp / 86400UL) + 5UL) % 7UL + 1UL);
}

uint32_t RTC_DateTimeToTimestamp(const rtc_date_t *date, const rtc_time_t *time) {
  uint32_t timestamp = 0U;

  if (date == NULL || time == NULL) return 0U;

  for (uint8_t y = 0U; y < date->year; y++) {
    timestamp += (uint32_t)(RTC_IsLeapYear(y) != 0U ? 366U : 365U) * 86400UL;
  }

  for (uint8_t m = 1U; m < date->month; m++) {
    timestamp += (uint32_t)RTC_GetDaysInMonth(m, date->year) * 86400UL;
  }

  timestamp += (uint32_t)(date->date - 1U) * 86400UL;
  timestamp += (uint32_t)time->hours * 3600UL;
  timestamp += (uint32_t)time->minutes * 60UL;
  timestamp += (uint32_t)time->seconds;
  return timestamp;
}

uint8_t RTC_IsLeapYear(uint8_t year) {
  uint16_t full_year = (uint16_t)(2000U + year);
  return ((full_year % 4U) == 0U &&
          (((full_year % 100U) != 0U) || ((full_year % 400U) == 0U)))
             ? 1U
             : 0U;
}

uint8_t RTC_GetDaysInMonth(uint8_t month, uint8_t year) {
  static const uint8_t k_days[13] = {0U, 31U, 28U, 31U, 30U, 31U, 30U, 31U,
                                     31U, 30U, 31U, 30U, 31U};

  if (month == 2U && RTC_IsLeapYear(year) != 0U) {
    return 29U;
  }

  if (month > 12U) {
    return 30U;
  }

  return k_days[month];
}

void RTC_AddSeconds(rtc_time_t *time, uint32_t seconds) {
  uint32_t total_seconds;

  if (time == NULL) return;

  total_seconds = ((uint32_t)time->hours * 3600UL) +
                  ((uint32_t)time->minutes * 60UL) +
                  (uint32_t)time->seconds + seconds;
  total_seconds %= 86400UL;

  time->hours = (uint8_t)(total_seconds / 3600UL);
  time->minutes = (uint8_t)((total_seconds % 3600UL) / 60UL);
  time->seconds = (uint8_t)(total_seconds % 60UL);
}

int32_t RTC_TimeDiff(const rtc_time_t *t1, const rtc_time_t *t2) {
  int32_t sec1;
  int32_t sec2;

  if (t1 == NULL || t2 == NULL) return 0;

  sec1 = (int32_t)t1->hours * 3600L + (int32_t)t1->minutes * 60L +
         (int32_t)t1->seconds;
  sec2 = (int32_t)t2->hours * 3600L + (int32_t)t2->minutes * 60L +
         (int32_t)t2->seconds;
  return sec1 - sec2;
}

void RTC_ProcessAlarmIRQ(void) {
  uint32_t isr = RTC->ISR;

  if ((isr & RTC_ISR_ALRAF) != 0U) {
    RTC_DisableWriteProtection();
    RTC_ClearAlarmFlags(1U);
    RTC_EnableWriteProtection();
    if (g_alarms[0].enabled != 0U && g_alarms[0].callback != NULL) {
      g_alarms[0].callback();
    }
  }

  if ((isr & RTC_ISR_ALRBF) != 0U) {
    RTC_DisableWriteProtection();
    RTC_ClearAlarmFlags(2U);
    RTC_EnableWriteProtection();
    if (g_alarms[1].enabled != 0U && g_alarms[1].callback != NULL) {
      g_alarms[1].callback();
    }
  }

  EXTI->PR = EXTI_PR_PR17;
}

void RTC_DebugSetDateTime(uint8_t year, uint8_t month, uint8_t date,
                          uint8_t weekday, uint8_t hour, uint8_t minute,
                          uint8_t second) {
  rtc_date_t rtc_date = {0};
  rtc_time_t rtc_time = {0};

  rtc_date.year = year;
  rtc_date.month = month;
  rtc_date.date = date;
  rtc_date.weekday = weekday;

  rtc_time.hours = hour;
  rtc_time.minutes = minute;
  rtc_time.seconds = second;

  RTC_SetTime(&rtc_time);
  RTC_SetDate(&rtc_date);
}

static uint8_t RTC_ToBCD(uint8_t value) {
  return (uint8_t)(((value / 10U) << 4U) | (value % 10U));
}

static uint8_t RTC_FromBCD(uint32_t value) {
  return (uint8_t)((((value >> 4U) & 0x0FU) * 10U) + (value & 0x0FU));
}

static uint8_t RTC_WaitForFlag(volatile uint32_t *reg, uint32_t mask,
                               uint32_t expected_state, uint32_t timeout_ms) {
  uint32_t start = HAL_GetTick();

  while (((*reg) & mask) != expected_state) {
    if ((HAL_GetTick() - start) >= timeout_ms) {
      return 0U;
    }
  }

  return 1U;
}

static void RTC_DisableWriteProtection(void) {
  RTC->WPR = 0xCAU;
  RTC->WPR = 0x53U;
}

static void RTC_EnableWriteProtection(void) { RTC->WPR = 0xFFU; }

static uint8_t RTC_EnterInitMode(void) {
  RTC->ISR |= RTC_ISR_INIT;
  return RTC_WaitForFlag(&RTC->ISR, RTC_ISR_INITF, RTC_ISR_INITF, 100U);
}

static void RTC_ExitInitMode(void) { RTC->ISR &= ~RTC_ISR_INIT; }

static void RTC_EnsureClockConfigured(void) {
  uint32_t bdcr = RCC->BDCR;
  uint32_t rtcsel = bdcr & RCC_BDCR_RTCSEL;
  uint8_t use_lse = 0U;

  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();

  if ((RTC->BKP0R == RTC_BKP_MAGIC) && ((bdcr & RCC_BDCR_RTCEN) != 0U) &&
      (rtcsel != 0U)) {
    return;
  }

  __HAL_RCC_LSE_CONFIG(RCC_LSE_ON);
  if (RTC_WaitForFlag(&RCC->BDCR, RCC_BDCR_LSERDY, RCC_BDCR_LSERDY,
                      RTC_INIT_TIMEOUT_MS) != 0U) {
    use_lse = 1U;
  } else {
    __HAL_RCC_LSI_ENABLE();
    (void)RTC_WaitForFlag(&RCC->CSR, RCC_CSR_LSIRDY, RCC_CSR_LSIRDY,
                          RTC_INIT_TIMEOUT_MS);
  }

  if (rtcsel != 0U && rtcsel != (use_lse != 0U ? RCC_BDCR_RTCSEL_0 : RCC_BDCR_RTCSEL_1)) {
    RCC->BDCR |= RCC_BDCR_BDRST;
    RCC->BDCR &= ~RCC_BDCR_BDRST;
  }

  __HAL_RCC_RTC_DISABLE();
  __HAL_RCC_RTC_CONFIG((use_lse != 0U) ? RCC_RTCCLKSOURCE_LSE : RCC_RTCCLKSOURCE_LSI);
  __HAL_RCC_RTC_ENABLE();

  RTC_DisableWriteProtection();
  if (RTC_EnterInitMode() != 0U) {
    RTC->CR &= ~(RTC_CR_FMT | RTC_CR_ALRAE | RTC_CR_ALRBE);
    RTC->PRER = ((use_lse != 0U ? 127U : 124U) << RTC_PRER_PREDIV_A_Pos) | 255U;
    RTC->TR = RTC_PackTime(&(rtc_time_t){0U, 0U, 0U});
    RTC->DR = RTC_PackDate(&(rtc_date_t){1U, 1U, 26U, 3U});
    RTC_ExitInitMode();
  }
  RTC_EnableWriteProtection();

  RTC->BKP0R = RTC_BKP_MAGIC;
}

static uint32_t RTC_PackTime(const rtc_time_t *time) {
  return ((uint32_t)(RTC_ToBCD(time->hours) & 0x3FU) << 16U) |
         ((uint32_t)(RTC_ToBCD(time->minutes) & 0x7FU) << 8U) |
         (uint32_t)(RTC_ToBCD(time->seconds) & 0x7FU);
}

static uint32_t RTC_PackDate(const rtc_date_t *date) {
  return ((uint32_t)(RTC_ToBCD(date->year) & 0xFFU) << 16U) |
         ((uint32_t)(date->weekday & 0x07U) << 13U) |
         ((uint32_t)(RTC_ToBCD(date->month) & 0x1FU) << 8U) |
         (uint32_t)(RTC_ToBCD(date->date) & 0x3FU);
}

static void RTC_UnpackTime(uint32_t tr, rtc_time_t *time) {
  time->hours = RTC_FromBCD((tr >> 16U) & 0x3FU);
  time->minutes = RTC_FromBCD((tr >> 8U) & 0x7FU);
  time->seconds = RTC_FromBCD(tr & 0x7FU);
}

static void RTC_UnpackDate(uint32_t dr, rtc_date_t *date) {
  date->year = RTC_FromBCD((dr >> 16U) & 0xFFU);
  date->weekday = (uint8_t)((dr >> 13U) & 0x07U);
  date->month = RTC_FromBCD((dr >> 8U) & 0x1FU);
  date->date = RTC_FromBCD(dr & 0x3FU);
}

static void RTC_ConfigureExtiAlarm(void) {
  EXTI->IMR |= EXTI_IMR_MR17;
  EXTI->RTSR |= EXTI_RTSR_TR17;
  EXTI->FTSR &= ~EXTI_FTSR_TR17;
  HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 6U, 0U);
  HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
}

static void RTC_ClearAlarmFlags(uint8_t alarm_id) {
  if (alarm_id == 0U || alarm_id > RTC_ALARM_COUNT) {
    RTC->ISR &= ~(RTC_ISR_ALRAF | RTC_ISR_ALRBF);
  } else if (alarm_id == 1U) {
    RTC->ISR &= ~RTC_ISR_ALRAF;
  } else {
    RTC->ISR &= ~RTC_ISR_ALRBF;
  }
}
