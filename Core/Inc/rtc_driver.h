/**
 ******************************************************************************
 * @file           : rtc_driver.h
 * @brief          : RTC Driver Header (Real-Time Clock for scheduling)
 ******************************************************************************
 */

#ifndef __RTC_DRIVER_H
#define __RTC_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* RTC Configuration ---------------------------------------------------------*/
#define RTC_ALARM_COUNT         2U      /* Number of supported alarms */

/* Time Structure -----------------------------------------------------------*/
typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} rtc_time_t;

/* Date Structure ------------------------------------------------------------*/
typedef struct {
    uint8_t date;
    uint8_t month;
    uint8_t year;     /* Last 2 digits of year */
    uint8_t weekday;  /* 1=Monday to 7=Sunday */
} rtc_date_t;

/* Alarm Structure -----------------------------------------------------------*/
typedef struct {
    uint8_t id;
    rtc_time_t time;
    uint8_t enabled;
    uint8_t daily_repeat;
    void (*callback)(void);
} rtc_alarm_t;

/* RTC Functions -------------------------------------------------------------*/
uint8_t RTC_Init(void);
uint8_t RTC_IsInitialized(void);

/* Time functions */
void RTC_GetTime(rtc_time_t *time);
void RTC_SetTime(const rtc_time_t *time);
void RTC_GetDate(rtc_date_t *date);
void RTC_SetDate(const rtc_date_t *date);

/* Alarm functions */
uint8_t RTC_SetAlarm(uint8_t alarm_id, const rtc_time_t *time, uint8_t daily_repeat);
uint8_t RTC_EnableAlarm(uint8_t alarm_id);
uint8_t RTC_DisableAlarm(uint8_t alarm_id);
void RTC_ClearAlarm(uint8_t alarm_id);
uint8_t RTC_GetAlarmState(uint8_t alarm_id);

/* Timestamp functions */
uint32_t RTC_GetTimestamp(void);
void RTC_TimestampToDateTime(uint32_t timestamp, rtc_date_t *date, rtc_time_t *time);
uint32_t RTC_DateTimeToTimestamp(const rtc_date_t *date, const rtc_time_t *time);

/* Utility functions */
uint8_t RTC_IsLeapYear(uint8_t year);
uint8_t RTC_GetDaysInMonth(uint8_t month, uint8_t year);
void RTC_AddSeconds(rtc_time_t *time, uint32_t seconds);
int32_t RTC_TimeDiff(const rtc_time_t *t1, const rtc_time_t *t2);
void RTC_ProcessAlarmIRQ(void);

#ifdef __cplusplus
}
#endif

#endif /* __RTC_DRIVER_H */
