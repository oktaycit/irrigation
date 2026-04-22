/**
 ******************************************************************************
 * @file           : rtc_driver.c
 * @brief          : RTC Driver Implementation (Real-Time Clock for scheduling)
 ******************************************************************************
 */

#include "main.h"
#include "rtc_driver.h"
#include <string.h>

extern RTC_HandleTypeDef hrtc;

static uint8_t g_rtc_initialized = 0;
static rtc_alarm_t g_alarms[RTC_ALARM_COUNT] = {0};

/* Days in each month (non-leap year) */
static const uint8_t days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/**
 * @brief  Initialize RTC
 */
uint8_t RTC_Init(void) {
    if (hrtc.Instance == RTC) {
        g_rtc_initialized = 1;
        
        /* Initialize alarms */
        memset(g_alarms, 0, sizeof(g_alarms));
        for (uint8_t i = 0; i < RTC_ALARM_COUNT; i++) {
            g_alarms[i].id = i + 1;
        }
        
        return 1;
    }
    return 0;
}

/**
 * @brief  Check if RTC is initialized
 */
uint8_t RTC_IsInitialized(void) {
    return g_rtc_initialized;
}

/**
 * @brief  Get current time
 */
void RTC_GetTime(rtc_time_t *time) {
    RTC_TimeTypeDef rtc_time;
    
    if (time == NULL || !g_rtc_initialized) return;
    
    HAL_RTC_GetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
    time->hours = rtc_time.Hours;
    time->minutes = rtc_time.Minutes;
    time->seconds = rtc_time.Seconds;
}

/**
 * @brief  Set current time
 */
void RTC_SetTime(const rtc_time_t *time) {
    RTC_TimeTypeDef rtc_time;
    
    if (time == NULL || !g_rtc_initialized) return;
    
    rtc_time.Hours = time->hours;
    rtc_time.Minutes = time->minutes;
    rtc_time.Seconds = time->seconds;
    rtc_time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    rtc_time.StoreOperation = RTC_STOREOPERATION_RESET;
    
    HAL_RTC_SetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);
}

/**
 * @brief  Get current date
 */
void RTC_GetDate(rtc_date_t *date) {
    RTC_DateTypeDef rtc_date;
    
    if (date == NULL || !g_rtc_initialized) return;
    
    HAL_RTC_GetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);
    date->date = rtc_date.Date;
    date->month = rtc_date.Month;
    date->year = rtc_date.Year;
    date->weekday = rtc_date.WeekDay;
}

/**
 * @brief  Set current date
 */
void RTC_SetDate(const rtc_date_t *date) {
    RTC_DateTypeDef rtc_date;
    
    if (date == NULL || !g_rtc_initialized) return;
    
    rtc_date.Date = date->date;
    rtc_date.Month = date->month;
    rtc_date.Year = date->year;
    rtc_date.WeekDay = date->weekday;
    
    HAL_RTC_SetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);
}

/**
 * @brief  Set alarm
 */
uint8_t RTC_SetAlarm(uint8_t alarm_id, const rtc_time_t *time, uint8_t daily_repeat) {
    RTC_AlarmTypeDef alarm;
    
    if (alarm_id == 0 || alarm_id > RTC_ALARM_COUNT || time == NULL || !g_rtc_initialized) {
        return 0;
    }
    
    /* Store alarm configuration */
    g_alarms[alarm_id - 1].time = *time;
    g_alarms[alarm_id - 1].daily_repeat = daily_repeat;
    g_alarms[alarm_id - 1].enabled = 0;  /* Disabled until explicitly enabled */
    
    /* Configure hardware alarm */
    alarm.Alarm = (alarm_id == 1) ? RTC_ALARM_A : RTC_ALARM_B;
    alarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;  /* Ignore date, alarm daily */
    alarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    
    if (daily_repeat) {
        alarm.AlarmMask |= RTC_ALARMMASK_DATEWEEKDAY;
    }
    
    alarm.AlarmTime.Hours = time->hours;
    alarm.AlarmTime.Minutes = time->minutes;
    alarm.AlarmTime.Seconds = time->seconds;
    alarm.AlarmTime.SubSeconds = 0;
    alarm.AlarmTime.TimeFormat = RTC_HOURFORMAT12_AM;
    alarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    
    /* Disable alarm first */
    HAL_RTC_DeactivateAlarm(&hrtc, alarm.Alarm);
    
    if (HAL_RTC_SetAlarm(&hrtc, &alarm, RTC_FORMAT_BIN) != HAL_OK) {
        return 0;
    }
    
    return 1;
}

/**
 * @brief  Enable alarm
 */
uint8_t RTC_EnableAlarm(uint8_t alarm_id) {
    if (alarm_id == 0 || alarm_id > RTC_ALARM_COUNT || !g_rtc_initialized) {
        return 0;
    }
    
    g_alarms[alarm_id - 1].enabled = 1;
    HAL_RTC_AlarmIRQHandler(&hrtc);
    __HAL_RTC_ALARM_ENABLE_IT(&hrtc, (alarm_id == 1) ? RTC_IT_ALRA : RTC_IT_ALRB);
    
    return 1;
}

/**
 * @brief  Disable alarm
 */
uint8_t RTC_DisableAlarm(uint8_t alarm_id) {
    if (alarm_id == 0 || alarm_id > RTC_ALARM_COUNT || !g_rtc_initialized) {
        return 0;
    }
    
    g_alarms[alarm_id - 1].enabled = 0;
    __HAL_RTC_ALARM_DISABLE_IT(&hrtc, (alarm_id == 1) ? RTC_IT_ALRA : RTC_IT_ALRB);
    
    return 1;
}

/**
 * @brief  Clear alarm flag
 */
void RTC_ClearAlarm(uint8_t alarm_id) {
    if (!g_rtc_initialized) return;
    
    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, (alarm_id == 1) ? RTC_FLAG_ALRAF : RTC_FLAG_ALRBF);
}

/**
 * @brief  Get alarm state
 */
uint8_t RTC_GetAlarmState(uint8_t alarm_id) {
    if (alarm_id == 0 || alarm_id > RTC_ALARM_COUNT) {
        return 0;
    }
    
    return g_alarms[alarm_id - 1].enabled;
}

/**
 * @brief  Get current timestamp (seconds since epoch)
 */
uint32_t RTC_GetTimestamp(void) {
    rtc_date_t date;
    rtc_time_t time;
    
    RTC_GetDate(&date);
    RTC_GetTime(&time);
    
    return RTC_DateTimeToTimestamp(&date, &time);
}

/**
 * @brief  Convert timestamp to date and time
 */
void RTC_TimestampToDateTime(uint32_t timestamp, rtc_date_t *date, rtc_time_t *time) {
    if (date == NULL || time == NULL) return;
    
    /* Simple conversion - assumes epoch 2000-01-01 00:00:00 */
    uint32_t days = timestamp / 86400;
    uint32_t remaining = timestamp % 86400;
    
    time->hours = remaining / 3600;
    time->minutes = (remaining % 3600) / 60;
    time->seconds = remaining % 60;
    
    /* Calculate date from days since epoch */
    date->year = 0;
    date->month = 1;
    date->date = 1;
    
    while (days >= (uint32_t)(RTC_IsLeapYear(date->year) ? 366 : 365)) {
        uint16_t year_days = RTC_IsLeapYear(date->year) ? 366 : 365;
        days -= year_days;
        date->year++;
    }
    
    while (days >= (uint32_t)RTC_GetDaysInMonth(date->month, date->year)) {
        days -= RTC_GetDaysInMonth(date->month, date->year);
        date->month++;
    }
    
    date->date += days;
    date->weekday = ((days + 1) % 7) + 1;  /* Jan 1, 2000 was Saturday (weekday 6) */
}

/**
 * @brief  Convert date and time to timestamp
 */
uint32_t RTC_DateTimeToTimestamp(const rtc_date_t *date, const rtc_time_t *time) {
    if (date == NULL || time == NULL) return 0;
    
    uint32_t timestamp = 0;
    
    /* Add years since 2000 */
    for (uint8_t y = 0; y < date->year; y++) {
        timestamp += (RTC_IsLeapYear(y) ? 366 : 365) * 86400;
    }
    
    /* Add months */
    for (uint8_t m = 1; m < date->month; m++) {
        timestamp += RTC_GetDaysInMonth(m, date->year) * 86400;
    }
    
    /* Add days */
    timestamp += (date->date - 1) * 86400;
    
    /* Add time */
    timestamp += time->hours * 3600 + time->minutes * 60 + time->seconds;
    
    return timestamp;
}

/**
 * @brief  Check if year is leap year
 */
uint8_t RTC_IsLeapYear(uint8_t year) {
    uint16_t full_year = 2000 + year;
    return ((full_year % 4 == 0 && full_year % 100 != 0) || (full_year % 400 == 0));
}

/**
 * @brief  Get days in month
 */
uint8_t RTC_GetDaysInMonth(uint8_t month, uint8_t year) {
    if (month == 2 && RTC_IsLeapYear(year)) {
        return 29;
    }
    return days_in_month[month];
}

/**
 * @brief  Add seconds to time
 */
void RTC_AddSeconds(rtc_time_t *time, uint32_t seconds) {
    if (time == NULL) return;
    
    uint32_t total_seconds = time->hours * 3600 + time->minutes * 60 + time->seconds + seconds;
    
    time->hours = (total_seconds / 3600) % 24;
    time->minutes = (total_seconds % 3600) / 60;
    time->seconds = total_seconds % 60;
}

/**
 * @brief  Calculate time difference in seconds (t1 - t2)
 */
int32_t RTC_TimeDiff(const rtc_time_t *t1, const rtc_time_t *t2) {
    if (t1 == NULL || t2 == NULL) return 0;
    
    int32_t sec1 = t1->hours * 3600 + t1->minutes * 60 + t1->seconds;
    int32_t sec2 = t2->hours * 3600 + t2->minutes * 60 + t2->seconds;
    
    return sec1 - sec2;
}

/* Alarm Callback Handler ----------------------------------------------------*/
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc) {
    (void)hrtc;
    
    /* Call user callback if set */
    if (g_alarms[0].enabled && g_alarms[0].callback != NULL) {
        g_alarms[0].callback();
    }
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) {
    (void)hrtc;
    
    /* Call user callback if set */
    if (g_alarms[1].enabled && g_alarms[1].callback != NULL) {
        g_alarms[1].callback();
    }
}
