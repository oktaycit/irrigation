/**
 ******************************************************************************
 * @file           : gui.h
 * @brief          : Grafiksel Kullanıcı Arayüzü (GUI) Driver
 ******************************************************************************
 */

#ifndef __GUI_H
#define __GUI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "lcd_ili9341.h"
#include "touch_xpt2046.h"
#include <stdbool.h>
#include <stdint.h>

/* GUI Configuration --------------------------------------------------------*/
#define GUI_MAX_SCREENS 20U
#define GUI_MAX_CONTROLS 32U
#define GUI_MAX_TEXT_LEN 64U

/* Ekran ID'leri ------------------------------------------------------------*/
typedef enum {
  SCREEN_MAIN = 0,        /* Ana Ekran */
  SCREEN_SETTINGS,        /* Ayarlar Menüsü */
  SCREEN_PH_SETTINGS,     /* pH Ayarları */
  SCREEN_EC_SETTINGS,     /* EC Ayarları */
  SCREEN_PARCEL_SETTINGS, /* Parsel Ayarları */
  SCREEN_MANUAL,          /* Manuel Kontrol */
  SCREEN_CALIBRATION,     /* Kalibrasyon */
  SCREEN_LOGS,            /* Log Kayıtları */
  SCREEN_SYSTEM_INFO,     /* Sistem Bilgisi */
  SCREEN_SPLASH,          /* Açılış Ekranı */
  SCREEN_WARNING,         /* Uyarı Ekranı */
  SCREEN_PROGRAMS,        /* Program Listesi */
  SCREEN_PROGRAM_EDIT,    /* Program Düzenleme */
  SCREEN_RTC_SETTINGS,    /* RTC Ayarlari */
  SCREEN_AUTO_MODE,       /* Otomatik Mod */
  SCREEN_ALARMS,          /* Alarm Ekrani */
  SCREEN_DOSING,          /* Dozaj Vanaları Kontrol */
  SCREEN_PARAMETERS,      /* Genel Parametreler */
  SCREEN_MAX
} screen_id_t;

/* Kontrol Tipleri ----------------------------------------------------------*/
typedef enum {
  CTRL_NONE = 0,
  CTRL_LABEL,
  CTRL_BUTTON,
  CTRL_TOGGLE,
  CTRL_SLIDER,
  CTRL_PROGRESS_BAR,
  CTRL_TEXT_BOX,
  CTRL_NUMBER_BOX,
  CTRL_ICON,
  CTRL_CHART
} control_type_t;

/* Kontrol Durumu -----------------------------------------------------------*/
typedef enum {
  CTRL_STATE_NORMAL = 0,
  CTRL_STATE_PRESSED,
  CTRL_STATE_DISABLED,
  CTRL_STATE_FOCUSED,
  CTRL_STATE_HIDDEN
} gui_control_state_t;

/* Event Tipleri ------------------------------------------------------------*/
typedef enum {
  EVENT_NONE = 0,
  EVENT_TOUCH_PRESSED,
  EVENT_TOUCH_RELEASED,
  EVENT_TOUCH_MOVED,
  EVENT_BUTTON_CLICKED,
  EVENT_VALUE_CHANGED,
  EVENT_SCREEN_LOADED,
  EVENT_SCREEN_UNLOADED,
  EVENT_TIMER_TICK
} event_type_t;

/* Event Yapısı -------------------------------------------------------------*/
typedef struct {
  event_type_t type;
  uint8_t control_id;
  int32_t value;
  touch_point_t touch_point;
} gui_event_t;

/* Callback Fonksiyon Tipi --------------------------------------------------*/
typedef void (*event_callback_t)(gui_event_t *event);

/* Label Control ------------------------------------------------------------*/
typedef struct {
  char text[GUI_MAX_TEXT_LEN];
  lcd_color_t fg_color;
  lcd_color_t bg_color;
  const lcd_font_t *font;
} label_control_t;

/* Button Control -----------------------------------------------------------*/
typedef struct {
  char text[GUI_MAX_TEXT_LEN];
  lcd_color_t normal_color;
  lcd_color_t pressed_color;
  lcd_color_t text_color;
  event_callback_t callback;
} button_control_t;

/* Slider Control -----------------------------------------------------------*/
typedef struct {
  int32_t min_value;
  int32_t max_value;
  int32_t current_value;
  uint8_t orientation; /* 0: yatay, 1: dikey */
  event_callback_t callback;
} slider_control_t;

/* Progress Bar Control -----------------------------------------------------*/
typedef struct {
  int32_t min_value;
  int32_t max_value;
  int32_t current_value;
  lcd_color_t fill_color;
  lcd_color_t bg_color;
  uint8_t show_text;
} progress_bar_control_t;

/* Number Box Control -------------------------------------------------------*/
typedef struct {
  int32_t min_value;
  int32_t max_value;
  int32_t current_value;
  uint8_t decimal_places;
  event_callback_t callback;
} number_box_control_t;

/* Control Base Yapısı ------------------------------------------------------*/
typedef struct {
  control_type_t type;
  gui_control_state_t state;
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
  uint8_t id;
  uint8_t visible;
  union {
    label_control_t label;
    button_control_t button;
    slider_control_t slider;
    progress_bar_control_t progress;
    number_box_control_t number;
  } data;
} gui_control_t;

/* Screen Yapısı ------------------------------------------------------------*/
typedef struct {
  screen_id_t id;
  char title[32];
  lcd_color_t bg_color;
  gui_control_t *controls;
  uint8_t control_count;
  void (*on_load)(void);
  void (*on_unload)(void);
  void (*on_update)(void);
  void (*on_event)(gui_event_t *event);
} gui_screen_t;

/* GUI Handle ---------------------------------------------------------------*/
typedef struct {
  gui_screen_t *screens[GUI_MAX_SCREENS];
  uint8_t screen_count;
  uint8_t current_screen;
  uint8_t is_initialized;
  uint32_t last_touch_time;
  touch_point_t last_touch;
} gui_handle_t;

/* GUI Fonksiyonları --------------------------------------------------------*/
void GUI_Init(void);
void GUI_Update(void);
void GUI_DrawScreen(screen_id_t screen_id);
screen_id_t GUI_GetCurrentScreen(void);
void GUI_NavigateTo(screen_id_t screen_id);
void GUI_Redraw(void);

/* Ekran Yönetimi -----------------------------------------------------------*/
void GUI_RegisterScreen(gui_screen_t *screen);
void GUI_SetTitle(const char *title);
void GUI_SetBackgroundColor(lcd_color_t color);

/* Dozaj Vana Kontrolü ------------------------------------------------------*/
void GUI_UpdateDosingValve(uint8_t valve_id, uint8_t is_open);

/* Kontrol Oluşturma --------------------------------------------------------*/
gui_control_t *GUI_CreateLabel(uint8_t id, uint16_t x, uint16_t y,
                               const char *text, lcd_color_t fg_color);
gui_control_t *GUI_CreateButton(uint8_t id, uint16_t x, uint16_t y, uint16_t w,
                                uint16_t h, const char *text,
                                event_callback_t callback);
gui_control_t *GUI_CreateSlider(uint8_t id, uint16_t x, uint16_t y, uint16_t w,
                                uint16_t h, int32_t min, int32_t max,
                                event_callback_t callback);
gui_control_t *GUI_CreateProgressBar(uint8_t id, uint16_t x, uint16_t y,
                                     uint16_t w, uint16_t h, int32_t min,
                                     int32_t max);
gui_control_t *GUI_CreateNumberBox(uint8_t id, uint16_t x, uint16_t y,
                                   uint16_t w, uint16_t h, int32_t min,
                                   int32_t max, event_callback_t callback);

/* Kontrol Güncelleme -------------------------------------------------------*/
void GUI_UpdateControl(gui_control_t *ctrl);
void GUI_SetControlVisible(uint8_t ctrl_id, uint8_t visible);
void GUI_SetControlValue(uint8_t ctrl_id, int32_t value);
int32_t GUI_GetControlValue(uint8_t ctrl_id);
void GUI_SetControlText(uint8_t ctrl_id, const char *text);
const char *GUI_GetControlText(uint8_t ctrl_id);

/* Touch Handling -----------------------------------------------------------*/
void GUI_ProcessTouch(touch_point_t *point);
uint8_t GUI_IsControlPressed(uint8_t ctrl_id);

/* Özel Çizim Fonksiyonları -----------------------------------------------*/
void GUI_DrawValueBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                      const char *label, float value, const char *unit,
                      lcd_color_t fg, lcd_color_t bg);
void GUI_DrawStatusbar(const char *left_text, const char *right_text);
void GUI_DrawHeader(const char *title);
void GUI_DrawButton(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                    const char *text, lcd_color_t color);

/* Ana Ekran Widget'ları ----------------------------------------------------*/
void GUI_DrawMainScreen(void);
void GUI_UpdatePHValue(float ph);
void GUI_UpdateECValue(float ec);
void GUI_UpdateValveStatus(uint8_t valve_id, uint8_t is_open);
void GUI_UpdateIrrigationStatus(uint8_t is_running, uint8_t parcel_id);

/* Fontlar ------------------------------------------------------------------*/
extern const lcd_font_t Font_8x5;
extern const lcd_font_t Font_16x8;
extern const lcd_font_t Font_24x16;
extern const lcd_font_t Font_32x24;

#ifdef __cplusplus
}
#endif

#endif /* __GUI_H */
