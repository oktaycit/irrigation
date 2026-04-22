/**
 ******************************************************************************
 * @file           : gui.c
 * @brief          : Grafiksel Kullanıcı Arayüzü (GUI) Driver Implementation
 ******************************************************************************
 */

#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GUI_HEADER_HEIGHT 30U
#define GUI_TOUCH_DEBOUNCE_MS 120U

typedef struct {
  ph_control_params_t ph_draft;
  ec_control_params_t ec_draft;
  uint8_t selected_parcel;
  uint32_t parcel_duration_sec;
  uint8_t parcel_enabled;
  uint8_t selected_program;
  uint8_t program_edit_page;
  irrigation_program_t program_draft;
  rtc_time_t rtc_time_draft;
  rtc_date_t rtc_date_draft;
  char notice[32];
} gui_runtime_state_t;

/* Internal Handle */
static gui_handle_t h_gui = {0};
static gui_runtime_state_t g_gui_state = {0};
static uint8_t g_gui_dirty = 1U;
static uint32_t g_gui_last_main_remaining_sec = UINT32_MAX;
static char g_gui_last_ph_text[32] = {0};
static char g_gui_last_ec_text[32] = {0};
static char g_gui_last_datetime_text[32] = {0};

static void GUI_DrawSplashScreen(void);
static void GUI_LoadScreenState(screen_id_t screen_id);
static void GUI_DrawMainActions(void);
static void GUI_DrawMainProgress(uint8_t parcel_id, uint32_t remaining_sec);
static void GUI_DrawManualScreen(void);
static void GUI_DrawSettingsScreen(void);
static void GUI_DrawPHSettingsScreen(void);
static void GUI_DrawECSettingsScreen(void);
static void GUI_DrawParcelSettingsScreen(void);
static void GUI_DrawCalibrationScreen(void);
static void GUI_DrawTouchCalibrationOverlay(void);
static void GUI_DrawSystemInfoScreen(void);
static void GUI_DrawProgramsScreen(void);
static void GUI_DrawProgramEditScreen(void);
static void GUI_DrawRTCSettingsScreen(void);
static void GUI_DrawAutoModeScreen(void);
static void GUI_DrawAlarmsScreen(void);
static void GUI_DrawBackButton(void);
static void GUI_DrawFooterButton(uint16_t x, const char *text,
                                 lcd_color_t color);
static void GUI_FormatValueText(char *buffer, size_t buffer_size, float value,
                                const char *unit);
static void GUI_FormatFixed2(char *buffer, size_t buffer_size, float value);
static void GUI_DrawValueTextArea(uint16_t x, uint16_t y, uint16_t w,
                                  uint16_t h, const char *value_text,
                                  lcd_color_t bg);
static uint16_t GUI_AdjustHHMM(uint16_t hhmm, int16_t delta_minutes);
#if BOARD_SENSOR_DEMO_MODE
static uint8_t GUI_GetDemoValueIndex(void);
static float GUI_GetDemoPHValue(void);
static float GUI_GetDemoECValue(void);
#endif
static void GUI_DrawAdjustRow(uint16_t y, const char *label, float value,
                              const char *unit);
static void GUI_DrawParcelAdjustRow(const char *label, const char *value,
                                    uint16_t y);
static void GUI_DrawMainModeLine(void);
static void GUI_FormatMainDateTime(char *buffer, size_t buffer_size);
static void GUI_DrawMainParcelLine(uint8_t current_parcel,
                                   uint32_t remaining_sec);
static void GUI_DrawManualValveButton(uint8_t valve_id);
static void GUI_DrawSystemInfoStateRow(void);
static void GUI_SetNotice(const char *text);
static const char *GUI_GetSystemHealthText(void);
static const char *GUI_GetAutoModeName(auto_mode_t mode);
static void GUI_FormatValveMask(char *buffer, size_t buffer_size, uint8_t mask);
static void GUI_FormatDaysMask(char *buffer, size_t buffer_size, uint8_t mask);
static uint8_t GUI_PointInRect(const touch_point_t *point, uint16_t x,
                               uint16_t y, uint16_t w, uint16_t h);
static void GUI_HandleMainTouch(const touch_point_t *point);
static void GUI_HandleManualTouch(const touch_point_t *point);
static void GUI_HandleSettingsTouch(const touch_point_t *point);
static void GUI_HandlePHSettingsTouch(const touch_point_t *point);
static void GUI_HandleECSettingsTouch(const touch_point_t *point);
static void GUI_HandleParcelSettingsTouch(const touch_point_t *point);
static void GUI_HandleCalibrationTouch(const touch_point_t *point);
static void GUI_HandleSystemInfoTouch(const touch_point_t *point);
static void GUI_HandleProgramsTouch(const touch_point_t *point);
static void GUI_HandleProgramEditTouch(const touch_point_t *point);
static void GUI_HandleRTCSettingsTouch(const touch_point_t *point);
static void GUI_HandleAutoModeTouch(const touch_point_t *point);
static void GUI_HandleAlarmsTouch(const touch_point_t *point);
static void GUI_RequestRedraw(void);

/**
 * @brief  Initialize GUI
 */
void GUI_Init(void) {
  memset(&h_gui, 0, sizeof(gui_handle_t));
  memset(&g_gui_state, 0, sizeof(gui_runtime_state_t));

  LCD_SetOrientation(LCD_ORIENTATION_LANDSCAPE);

  h_gui.is_initialized = 1U;
  h_gui.current_screen = SCREEN_SPLASH;
  g_gui_state.selected_parcel = 1U;

  GUI_DrawScreen(SCREEN_SPLASH);
  HAL_Delay(1000);

  GUI_NavigateTo(SCREEN_MAIN);
}

/**
 * @brief  GUI Periodic Update
 */
void GUI_Update(void) {
  touch_point_t point = {0};

  if (h_gui.is_initialized == 0U) return;

  if (TOUCH_ReadPoint(&point) != 0U) {
    if ((h_gui.last_touch.pressed == 0U) &&
        ((HAL_GetTick() - h_gui.last_touch_time) >= GUI_TOUCH_DEBOUNCE_MS)) {
      if ((h_gui.current_screen == SCREEN_CALIBRATION) &&
          (TOUCH_IsCalibrating() != 0U)) {
        GUI_ProcessTouch(&point);
      } else {
      GUI_ProcessTouch(&point);
      }
      h_gui.last_touch_time = HAL_GetTick();
    }
    h_gui.last_touch = point;
  } else {
    memset(&h_gui.last_touch, 0, sizeof(h_gui.last_touch));
  }

  if (h_gui.current_screen == SCREEN_MAIN && g_gui_dirty == 0U) {
    uint32_t remaining_sec = IRRIGATION_CTRL_GetRemainingTime();
    char datetime_text[32] = {0};

    if (remaining_sec != g_gui_last_main_remaining_sec) {
      g_gui_last_main_remaining_sec = remaining_sec;
      GUI_DrawMainParcelLine(IRRIGATION_CTRL_GetCurrentParcelId(), remaining_sec);
      GUI_DrawMainProgress(IRRIGATION_CTRL_GetCurrentParcelId(), remaining_sec);
    }

    GUI_FormatMainDateTime(datetime_text, sizeof(datetime_text));
    if (strcmp(datetime_text, g_gui_last_datetime_text) != 0) {
      snprintf(g_gui_last_datetime_text, sizeof(g_gui_last_datetime_text), "%s",
               datetime_text);
      GUI_DrawMainModeLine();
    }

#if BOARD_SENSOR_DEMO_MODE
    GUI_UpdatePHValue(GUI_GetDemoPHValue());
    GUI_UpdateECValue(GUI_GetDemoECValue());
#endif
  }

  if (g_gui_dirty != 0U) {
    GUI_Redraw();
  }
}

/**
 * @brief  Navigate to a specific screen
 */
void GUI_NavigateTo(screen_id_t screen_id) {
  if (screen_id >= SCREEN_MAX) return;

  GUI_LoadScreenState(screen_id);
  h_gui.current_screen = screen_id;
  g_gui_last_main_remaining_sec = UINT32_MAX;
  memset(g_gui_last_datetime_text, 0, sizeof(g_gui_last_datetime_text));
  GUI_RequestRedraw();
  GUI_DrawScreen(screen_id);
}

/**
 * @brief  Draw Main Screen Layout
 */
void GUI_DrawMainScreen(void) {
  uint8_t current_parcel = IRRIGATION_CTRL_GetCurrentParcelId();
  uint32_t remaining_sec = IRRIGATION_CTRL_GetRemainingTime();
  uint16_t screen_width = LCD_GetDisplayWidth();
#if BOARD_SENSOR_DEMO_MODE
  float ph_value = GUI_GetDemoPHValue();
  float ec_value = GUI_GetDemoECValue();
  const char *ph_label = "PH DEMO";
  const char *ec_label = "EC DEMO";
#else
  float ph_value = IRRIGATION_CTRL_GetPH();
  float ec_value = IRRIGATION_CTRL_GetEC();
  const char *ph_label = "PH";
  const char *ec_label = "EC";
#endif

  LCD_Clear(ILI9341_BLACK);

#if BOARD_SENSOR_DEMO_MODE
  GUI_DrawStatusbar(IRRIGATION_CTRL_GetStateName(IRRIGATION_CTRL_GetState()),
                    "DEMO");
#else
  GUI_DrawStatusbar(IRRIGATION_CTRL_GetStateName(IRRIGATION_CTRL_GetState()),
                    GUI_GetSystemHealthText());
#endif

  GUI_DrawValueBox(10U, 42U, 145U, 56U, ph_label, ph_value, "",
                   ILI9341_CYAN, ILI9341_BLACK);
  GUI_DrawValueBox(165U, 42U, 145U, 56U, ec_label, ec_value, "MS",
                   ILI9341_GREEN, ILI9341_BLACK);

  GUI_DrawMainModeLine();
  GUI_DrawMainParcelLine(current_parcel, remaining_sec);

  GUI_DrawMainProgress(current_parcel, remaining_sec);
  GUI_DrawMainActions();

  LCD_DrawHLine(0U, 185U, screen_width, ILI9341_DARKGRAY);
}

/**
 * @brief  Draw a simple button
 */
void GUI_DrawButton(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                    const char *text, lcd_color_t color) {
  size_t text_len = strlen(text);
  uint16_t text_width = (uint16_t)(text_len * Font_16x8.width);
  uint16_t text_x = x + 6U;

  LCD_FillRect(x, y, w, h, color);
  LCD_DrawRect(x, y, w, h, ILI9341_WHITE);

  if (text_width < w) {
    text_x = x + (uint16_t)((w - text_width) / 2U);
  }

  LCD_DrawString(text_x, y + (uint16_t)((h - Font_16x8.height) / 2U), text,
                 ILI9341_WHITE, color, &Font_16x8);
}

/**
 * @brief  Process touch events
 */
void GUI_ProcessTouch(touch_point_t *point) {
  if (point == NULL || point->pressed == 0U) return;

  switch (h_gui.current_screen) {
  case SCREEN_MAIN:
    GUI_HandleMainTouch(point);
    break;
  case SCREEN_MANUAL:
    GUI_HandleManualTouch(point);
    break;
  case SCREEN_SETTINGS:
    GUI_HandleSettingsTouch(point);
    break;
  case SCREEN_PH_SETTINGS:
    GUI_HandlePHSettingsTouch(point);
    break;
  case SCREEN_EC_SETTINGS:
    GUI_HandleECSettingsTouch(point);
    break;
  case SCREEN_PARCEL_SETTINGS:
    GUI_HandleParcelSettingsTouch(point);
    break;
  case SCREEN_CALIBRATION:
    GUI_HandleCalibrationTouch(point);
    break;
  case SCREEN_SYSTEM_INFO:
    GUI_HandleSystemInfoTouch(point);
    break;
  case SCREEN_PROGRAMS:
    GUI_HandleProgramsTouch(point);
    break;
  case SCREEN_PROGRAM_EDIT:
    GUI_HandleProgramEditTouch(point);
    break;
  case SCREEN_RTC_SETTINGS:
    GUI_HandleRTCSettingsTouch(point);
    break;
  case SCREEN_AUTO_MODE:
    GUI_HandleAutoModeTouch(point);
    break;
  case SCREEN_ALARMS:
    GUI_HandleAlarmsTouch(point);
    break;
  default:
    break;
  }
}

/**
 * @brief  Draw common header
 */
void GUI_DrawHeader(const char *title) {
  LCD_FillRect(0U, 0U, LCD_GetDisplayWidth(), GUI_HEADER_HEIGHT, ILI9341_NAVY);
  LCD_DrawString(10U, 7U, title, ILI9341_WHITE, ILI9341_NAVY, &Font_16x8);
}

void GUI_DrawScreen(screen_id_t screen_id) {
  switch (screen_id) {
  case SCREEN_SPLASH:
    GUI_DrawSplashScreen();
    break;
  case SCREEN_MAIN:
    GUI_DrawMainScreen();
    break;
  case SCREEN_MANUAL:
    GUI_DrawManualScreen();
    break;
  case SCREEN_SETTINGS:
    GUI_DrawSettingsScreen();
    break;
  case SCREEN_PH_SETTINGS:
    GUI_DrawPHSettingsScreen();
    break;
  case SCREEN_EC_SETTINGS:
    GUI_DrawECSettingsScreen();
    break;
  case SCREEN_PARCEL_SETTINGS:
    GUI_DrawParcelSettingsScreen();
    break;
  case SCREEN_CALIBRATION:
    GUI_DrawCalibrationScreen();
    break;
  case SCREEN_SYSTEM_INFO:
    GUI_DrawSystemInfoScreen();
    break;
  case SCREEN_PROGRAMS:
    GUI_DrawProgramsScreen();
    break;
  case SCREEN_PROGRAM_EDIT:
    GUI_DrawProgramEditScreen();
    break;
  case SCREEN_RTC_SETTINGS:
    GUI_DrawRTCSettingsScreen();
    break;
  case SCREEN_AUTO_MODE:
    GUI_DrawAutoModeScreen();
    break;
  case SCREEN_ALARMS:
    GUI_DrawAlarmsScreen();
    break;
  default:
    GUI_DrawHeader("MENU");
    break;
  }
}

screen_id_t GUI_GetCurrentScreen(void) {
  return (screen_id_t)h_gui.current_screen;
}

void GUI_Redraw(void) {
  g_gui_dirty = 0U;
  GUI_DrawScreen((screen_id_t)h_gui.current_screen);
  if (h_gui.current_screen == SCREEN_MAIN) {
    g_gui_last_main_remaining_sec = IRRIGATION_CTRL_GetRemainingTime();
  }
}

void GUI_DrawValueBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                      const char *label, float value, const char *unit,
                      lcd_color_t fg, lcd_color_t bg) {
  char value_text[32];
  char *cache = NULL;

  if ((x == 10U) && (y == 42U)) {
    cache = g_gui_last_ph_text;
  } else if ((x == 165U) && (y == 42U)) {
    cache = g_gui_last_ec_text;
  }

  LCD_FillRect(x, y, w, h, bg);
  LCD_DrawRect(x, y, w, h, fg);
  LCD_DrawString(x + 8U, y + 8U, label, fg, bg, &Font_16x8);
  GUI_FormatValueText(value_text, sizeof(value_text), value, unit);
  GUI_DrawValueTextArea(x, y, w, h, value_text, bg);

  if (cache != NULL) {
    snprintf(cache, 32U, "%s", value_text);
  }
}

void GUI_DrawStatusbar(const char *left_text, const char *right_text) {
  uint16_t screen_width = LCD_GetDisplayWidth();
  uint16_t right_x = 170U;

  LCD_FillRect(0U, 0U, screen_width, 24U, ILI9341_DARKGRAY);
  LCD_DrawString(8U, 4U, left_text, ILI9341_WHITE, ILI9341_DARKGRAY,
                 &Font_16x8);

  if ((right_text != NULL) && (strlen(right_text) < 14U)) {
    uint16_t estimated_width = (uint16_t)(strlen(right_text) * Font_16x8.width);
    if (estimated_width + 10U < screen_width) {
      right_x = screen_width - estimated_width - 8U;
    }
  }

  LCD_DrawString(right_x, 4U, right_text, ILI9341_CYAN, ILI9341_DARKGRAY,
                 &Font_16x8);
}

void GUI_UpdatePHValue(float ph) {
  char value_text[32];

  if (h_gui.is_initialized == 0U) return;
  GUI_FormatValueText(value_text, sizeof(value_text), ph, "");
  if (strcmp(value_text, g_gui_last_ph_text) == 0) return;

  if (h_gui.current_screen == SCREEN_MAIN && g_gui_dirty == 0U) {
    GUI_DrawValueTextArea(10U, 42U, 145U, 56U, value_text, ILI9341_BLACK);
    snprintf(g_gui_last_ph_text, sizeof(g_gui_last_ph_text), "%s", value_text);
  } else if (h_gui.current_screen == SCREEN_MAIN) {
    GUI_RequestRedraw();
  }
}

void GUI_UpdateECValue(float ec) {
  char value_text[32];

  if (h_gui.is_initialized == 0U) return;
  GUI_FormatValueText(value_text, sizeof(value_text), ec, "MS");
  if (strcmp(value_text, g_gui_last_ec_text) == 0) return;

  if (h_gui.current_screen == SCREEN_MAIN && g_gui_dirty == 0U) {
    GUI_DrawValueTextArea(165U, 42U, 145U, 56U, value_text, ILI9341_BLACK);
    snprintf(g_gui_last_ec_text, sizeof(g_gui_last_ec_text), "%s", value_text);
  } else if (h_gui.current_screen == SCREEN_MAIN) {
    GUI_RequestRedraw();
  }
}

void GUI_UpdateValveStatus(uint8_t valve_id, uint8_t is_open) {
  (void)is_open;
  if (h_gui.is_initialized == 0U) return;
  if (valve_id == 0U || valve_id > VALVE_COUNT) return;

  if (h_gui.current_screen == SCREEN_MANUAL && g_gui_dirty == 0U) {
    GUI_DrawManualValveButton(valve_id);
  } else if (h_gui.current_screen == SCREEN_MANUAL) {
    GUI_RequestRedraw();
  }
}

void GUI_UpdateIrrigationStatus(uint8_t is_running, uint8_t parcel_id) {
  uint32_t remaining_sec = IRRIGATION_CTRL_GetRemainingTime();

  (void)is_running;
  if (h_gui.is_initialized == 0U) return;

  if (h_gui.current_screen == SCREEN_MAIN && g_gui_dirty == 0U) {
    g_gui_last_main_remaining_sec = remaining_sec;
    GUI_DrawStatusbar(IRRIGATION_CTRL_GetStateName(IRRIGATION_CTRL_GetState()),
                      GUI_GetSystemHealthText());
    GUI_DrawMainModeLine();
    GUI_DrawMainParcelLine(parcel_id, remaining_sec);
    GUI_DrawMainProgress(parcel_id, remaining_sec);
    GUI_DrawMainActions();
  } else if (h_gui.current_screen == SCREEN_SYSTEM_INFO && g_gui_dirty == 0U) {
    GUI_DrawSystemInfoStateRow();
  } else if (h_gui.current_screen == SCREEN_MAIN ||
             h_gui.current_screen == SCREEN_SYSTEM_INFO) {
    GUI_RequestRedraw();
  }
}

static void GUI_RequestRedraw(void) { g_gui_dirty = 1U; }

static void GUI_FormatValueText(char *buffer, size_t buffer_size, float value,
                                const char *unit) {
  char value_buffer[20];

  if (buffer == NULL || buffer_size == 0U) return;

  GUI_FormatFixed2(value_buffer, sizeof(value_buffer), value);

  if (unit != NULL && unit[0] != '\0') {
    snprintf(buffer, buffer_size, "%s %s", value_buffer, unit);
  } else {
    snprintf(buffer, buffer_size, "%s", value_buffer);
  }
}

static void GUI_FormatFixed2(char *buffer, size_t buffer_size, float value) {
  int32_t scaled;
  uint32_t abs_scaled;
  uint32_t integer_part;
  uint32_t fraction_part;

  if (buffer == NULL || buffer_size == 0U) return;

  if (value >= 0.0f) {
    scaled = (int32_t)((value * 100.0f) + 0.5f);
  } else {
    scaled = (int32_t)((value * 100.0f) - 0.5f);
  }

  abs_scaled = (scaled < 0) ? (uint32_t)(-scaled) : (uint32_t)scaled;
  integer_part = abs_scaled / 100U;
  fraction_part = abs_scaled % 100U;

  if (scaled < 0) {
    snprintf(buffer, buffer_size, "-%lu.%02lu", (unsigned long)integer_part,
             (unsigned long)fraction_part);
  } else {
    snprintf(buffer, buffer_size, "%lu.%02lu", (unsigned long)integer_part,
             (unsigned long)fraction_part);
  }
}

static void GUI_DrawValueTextArea(uint16_t x, uint16_t y, uint16_t w,
                                  uint16_t h, const char *value_text,
                                  lcd_color_t bg) {
  uint16_t value_y = y + 30U;
  uint16_t value_h = Font_16x8.height;

  (void)h;
  LCD_FillRect(x + 8U, value_y, w - 16U, value_h, bg);
  LCD_DrawString(x + 8U, value_y, value_text, ILI9341_WHITE, bg, &Font_16x8);
}

#if BOARD_SENSOR_DEMO_MODE
static uint8_t GUI_GetDemoValueIndex(void) {
  return (uint8_t)((HAL_GetTick() / 1000U) % 5U);
}

static float GUI_GetDemoPHValue(void) {
  static const float demo_values[] = {4.44f, 5.55f, 6.66f, 7.77f, 8.88f};
  return demo_values[GUI_GetDemoValueIndex()];
}

static float GUI_GetDemoECValue(void) {
  static const float demo_values[] = {0.44f, 1.11f, 2.22f, 3.33f, 4.44f};
  return demo_values[GUI_GetDemoValueIndex()];
}
#endif

static void GUI_LoadScreenState(screen_id_t screen_id) {
  switch (screen_id) {
  case SCREEN_PH_SETTINGS:
    IRRIGATION_CTRL_GetPHParams(&g_gui_state.ph_draft);
    break;
  case SCREEN_EC_SETTINGS:
    IRRIGATION_CTRL_GetECParams(&g_gui_state.ec_draft);
    break;
  case SCREEN_PARCEL_SETTINGS:
    if (g_gui_state.selected_parcel == 0U ||
        g_gui_state.selected_parcel > VALVE_COUNT) {
      g_gui_state.selected_parcel = 1U;
    }
    g_gui_state.parcel_duration_sec =
        PARCELS_GetDuration(g_gui_state.selected_parcel);
    g_gui_state.parcel_enabled =
        PARCELS_IsEnabled(g_gui_state.selected_parcel);
    break;
  case SCREEN_CALIBRATION:
    GUI_SetNotice("READY");
    break;
  case SCREEN_PROGRAMS:
    if (g_gui_state.selected_program == 0U ||
        g_gui_state.selected_program > VALVE_COUNT) {
      g_gui_state.selected_program = 1U;
    }
    break;
  case SCREEN_PROGRAM_EDIT:
    if (g_gui_state.selected_program == 0U ||
        g_gui_state.selected_program > VALVE_COUNT) {
      g_gui_state.selected_program = 1U;
    }
    IRRIGATION_CTRL_GetProgram(g_gui_state.selected_program,
                               &g_gui_state.program_draft);
    break;
  case SCREEN_RTC_SETTINGS:
    RTC_GetTime(&g_gui_state.rtc_time_draft);
    RTC_GetDate(&g_gui_state.rtc_date_draft);
    break;
  default:
    break;
  }
}

static void GUI_DrawSplashScreen(void) {
  LCD_Clear(ILI9341_BLACK);
  LCD_DrawString(52U, 72U, "STM32 IRRIGATION", ILI9341_GREEN, ILI9341_BLACK,
                 &Font_24x16);
  LCD_DrawString(84U, 116U, "CONTROL", ILI9341_CYAN, ILI9341_BLACK,
                 &Font_24x16);
}

static void GUI_DrawBackButton(void) {
  GUI_DrawButton(LCD_GetDisplayWidth() - 72U, 4U, 64U, 22U, "BACK",
                 ILI9341_DARKGRAY);
}

static void GUI_DrawFooterButton(uint16_t x, const char *text,
                                 lcd_color_t color) {
  GUI_DrawButton(x, 202U, 145U, 28U, text, color);
}

static void GUI_DrawMainModeLine(void) {
  char text[32];
  char datetime_text[32];
  uint16_t text_x = 0U;
  uint16_t text_width = 0U;

  LCD_FillRect(0U, 108U, LCD_GetDisplayWidth(), 16U, ILI9341_BLACK);
  snprintf(text, sizeof(text), "MODE %s",
           GUI_GetAutoModeName(IRRIGATION_CTRL_GetAutoMode()));
  LCD_DrawString(10U, 108U, text, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);

  GUI_FormatMainDateTime(datetime_text, sizeof(datetime_text));
  snprintf(g_gui_last_datetime_text, sizeof(g_gui_last_datetime_text), "%s",
           datetime_text);
  text_width = (uint16_t)(strlen(datetime_text) * Font_16x8.width);
  if (text_width < LCD_GetDisplayWidth()) {
    text_x = (uint16_t)(LCD_GetDisplayWidth() - text_width - 10U);
  }
  LCD_DrawString(text_x, 108U, datetime_text, ILI9341_CYAN, ILI9341_BLACK,
                 &Font_16x8);
}

static void GUI_FormatMainDateTime(char *buffer, size_t buffer_size) {
  rtc_time_t time = {0};
  rtc_date_t date = {0};

  if (buffer == NULL || buffer_size == 0U) {
    return;
  }

  if (RTC_IsInitialized() == 0U) {
    snprintf(buffer, buffer_size, "--/--/-- --:--");
    return;
  }

  RTC_GetTime(&time);
  RTC_GetDate(&date);
  snprintf(buffer, buffer_size, "%u/%u/%02u %02u:%02u",
           (unsigned int)date.date, (unsigned int)date.month,
           (unsigned int)date.year, (unsigned int)time.hours,
           (unsigned int)time.minutes);
}

static void GUI_DrawMainParcelLine(uint8_t current_parcel,
                                   uint32_t remaining_sec) {
  char text[32];

  LCD_FillRect(0U, 130U, LCD_GetDisplayWidth(), 16U, ILI9341_BLACK);

  if (IRRIGATION_CTRL_IsRunning() != 0U && current_parcel != 0U) {
    snprintf(text, sizeof(text), "PARCEL %u", current_parcel);
    LCD_DrawString(10U, 130U, text, ILI9341_YELLOW, ILI9341_BLACK, &Font_16x8);

    snprintf(text, sizeof(text), "%02lu:%02lu LEFT",
             (unsigned long)(remaining_sec / 60U),
             (unsigned long)(remaining_sec % 60U));
    LCD_DrawString(180U, 130U, text, ILI9341_WHITE, ILI9341_BLACK,
                   &Font_16x8);
  } else {
    LCD_DrawString(10U, 130U, "SYSTEM IDLE", ILI9341_GRAY, ILI9341_BLACK,
                   &Font_16x8);
  }
}

static void GUI_DrawMainActions(void) {
  const char *run_text =
      (IRRIGATION_CTRL_IsRunning() != 0U) ? "STOP" : "START";
  lcd_color_t run_color =
      (IRRIGATION_CTRL_IsRunning() != 0U) ? ILI9341_DARKRED : ILI9341_DARKGREEN;

  GUI_DrawButton(10U, 195U, 96U, 34U, run_text, run_color);
  GUI_DrawButton(112U, 195U, 96U, 34U, "MANUAL", ILI9341_DARKGRAY);
  GUI_DrawButton(214U, 195U, 96U, 34U, "SETTINGS", ILI9341_NAVY);
}

static void GUI_DrawMainProgress(uint8_t parcel_id, uint32_t remaining_sec) {
  char text[24];
  uint32_t duration_sec = 0U;
  uint32_t progress_width = 0U;

  LCD_FillRect(10U, 152U, 300U, 20U, ILI9341_BLACK);
  LCD_DrawRect(10U, 152U, 300U, 20U, ILI9341_GRAY);

  if (parcel_id != 0U) {
    duration_sec = PARCELS_GetDuration(parcel_id);
    if (duration_sec > remaining_sec && duration_sec != 0U) {
      progress_width =
          (uint32_t)(298U * (duration_sec - remaining_sec) / duration_sec);
    }
  }

  if (progress_width > 0U) {
    LCD_FillRect(11U, 153U, (uint16_t)progress_width, 18U, ILI9341_BLUE);
  }

  if (parcel_id != 0U && duration_sec != 0U) {
    snprintf(text, sizeof(text), "%lu%%",
             (unsigned long)(((duration_sec - remaining_sec) * 100U) /
                             duration_sec));
  } else {
    snprintf(text, sizeof(text), "READY");
  }

  LCD_DrawString(132U, 155U, text, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);
}

static void GUI_DrawManualScreen(void) {
  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("MANUAL VALVES");
  GUI_DrawBackButton();

  for (uint8_t valve_id = 1U; valve_id <= VALVE_COUNT; valve_id++) {
    GUI_DrawManualValveButton(valve_id);
  }

  GUI_DrawFooterButton(10U, "ALL OFF", ILI9341_DARKRED);
  GUI_DrawFooterButton(165U, "MAIN", ILI9341_NAVY);
}

static void GUI_DrawSettingsScreen(void) {
  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("SETTINGS");
  GUI_DrawBackButton();

  GUI_DrawButton(10U, 38U, 145U, 30U, "PH", ILI9341_DARKGRAY);
  GUI_DrawButton(165U, 38U, 145U, 30U, "EC", ILI9341_DARKGRAY);
  GUI_DrawButton(10U, 74U, 145U, 30U, "PARCEL", ILI9341_DARKGRAY);
  GUI_DrawButton(165U, 74U, 145U, 30U, "CALIB", ILI9341_DARKGRAY);
  GUI_DrawButton(10U, 110U, 145U, 30U, "PROGRAM", ILI9341_DARKGRAY);
  GUI_DrawButton(165U, 110U, 145U, 30U, "RTC", ILI9341_DARKGRAY);
  GUI_DrawButton(10U, 146U, 145U, 30U, "AUTO", ILI9341_DARKGRAY);
  GUI_DrawButton(165U, 146U, 145U, 30U, "ALARM", ILI9341_DARKGRAY);
  GUI_DrawFooterButton(10U, "INFO", ILI9341_DARKGRAY);
  GUI_DrawFooterButton(165U, "MAIN", ILI9341_NAVY);
}

static void GUI_DrawAdjustRow(uint16_t y, const char *label, float value,
                              const char *unit) {
  char value_text[24];

  LCD_DrawString(10U, y + 8U, label, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);
  LCD_DrawRect(110U, y, 90U, 30U, ILI9341_GRAY);

  if (unit != NULL && unit[0] != '\0') {
    snprintf(value_text, sizeof(value_text), "%.2f %s", value, unit);
  } else {
    snprintf(value_text, sizeof(value_text), "%.2f", value);
  }

  LCD_DrawString(118U, y + 7U, value_text, ILI9341_CYAN, ILI9341_BLACK,
                 &Font_16x8);
  GUI_DrawButton(215U, y, 40U, 30U, "-", ILI9341_DARKGRAY);
  GUI_DrawButton(265U, y, 40U, 30U, "+", ILI9341_DARKGRAY);
}

static void GUI_DrawPHSettingsScreen(void) {
  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("PH SETTINGS");
  GUI_DrawBackButton();

  GUI_DrawAdjustRow(46U, "TARGET", g_gui_state.ph_draft.target, "");
  GUI_DrawAdjustRow(88U, "MIN", g_gui_state.ph_draft.min_limit, "");
  GUI_DrawAdjustRow(130U, "MAX", g_gui_state.ph_draft.max_limit, "");

  GUI_DrawFooterButton(10U, "APPLY", ILI9341_DARKGREEN);
  GUI_DrawFooterButton(165U, "BACK", ILI9341_NAVY);
}

static void GUI_DrawECSettingsScreen(void) {
  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("EC SETTINGS");
  GUI_DrawBackButton();

  GUI_DrawAdjustRow(46U, "TARGET", g_gui_state.ec_draft.target, "MS");
  GUI_DrawAdjustRow(88U, "MIN", g_gui_state.ec_draft.min_limit, "MS");
  GUI_DrawAdjustRow(130U, "MAX", g_gui_state.ec_draft.max_limit, "MS");

  GUI_DrawFooterButton(10U, "APPLY", ILI9341_DARKGREEN);
  GUI_DrawFooterButton(165U, "BACK", ILI9341_NAVY);
}

static void GUI_DrawParcelAdjustRow(const char *label, const char *value,
                                    uint16_t y) {
  LCD_DrawString(10U, y + 8U, label, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);
  LCD_DrawRect(110U, y, 90U, 30U, ILI9341_GRAY);
  LCD_DrawString(118U, y + 7U, value, ILI9341_CYAN, ILI9341_BLACK, &Font_16x8);
  GUI_DrawButton(215U, y, 40U, 30U, "-", ILI9341_DARKGRAY);
  GUI_DrawButton(265U, y, 40U, 30U, "+", ILI9341_DARKGRAY);
}

static void GUI_DrawParcelSettingsScreen(void) {
  char parcel_text[24];
  char duration_text[24];
  char enabled_text[24];

  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("PARCEL SETTINGS");
  GUI_DrawBackButton();

  snprintf(parcel_text, sizeof(parcel_text), "P%u", g_gui_state.selected_parcel);
  snprintf(duration_text, sizeof(duration_text), "%lu S",
           (unsigned long)g_gui_state.parcel_duration_sec);
  snprintf(enabled_text, sizeof(enabled_text), "%s",
           (g_gui_state.parcel_enabled != 0U) ? "ON" : "OFF");

  GUI_DrawParcelAdjustRow("PARCEL", parcel_text, 46U);
  GUI_DrawParcelAdjustRow("DURATION", duration_text, 88U);
  GUI_DrawParcelAdjustRow("ENABLED", enabled_text, 130U);

  GUI_DrawFooterButton(10U, "APPLY", ILI9341_DARKGREEN);
  GUI_DrawFooterButton(165U, "BACK", ILI9341_NAVY);
}

static void GUI_DrawCalibrationScreen(void) {
  if (TOUCH_IsCalibrating() != 0U) {
    GUI_DrawTouchCalibrationOverlay();
    return;
  }

  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("CALIBRATION");
  GUI_DrawBackButton();

  GUI_DrawButton(10U, 48U, 145U, 42U, "TOUCH CAL", ILI9341_DARKGRAY);
  GUI_DrawButton(165U, 48U, 145U, 42U, "PH CAL", ILI9341_DARKGRAY);
  GUI_DrawButton(10U, 100U, 145U, 42U, "EC CAL", ILI9341_DARKGRAY);
  GUI_DrawButton(165U, 100U, 145U, 42U, "MAIN", ILI9341_NAVY);

  LCD_DrawRect(10U, 160U, 300U, 44U, ILI9341_DARKGRAY);
  LCD_DrawString(18U, 172U, g_gui_state.notice, ILI9341_YELLOW, ILI9341_BLACK,
                 &Font_16x8);
}

static void GUI_DrawTouchCalibrationOverlay(void) {
  uint16_t x = 24U;
  uint16_t y = 32U;
  const char *detail = "TOP LEFT";

  switch (TOUCH_GetCalibrationStep()) {
  case 2U:
    x = LCD_GetDisplayWidth() - 25U;
    y = 32U;
    detail = "TOP RIGHT";
    break;
  case 3U:
    x = 24U;
    y = LCD_GetDisplayHeight() - 25U;
    detail = "BOTTOM LEFT";
    break;
  default:
    break;
  }

  LCD_Clear(ILI9341_BLACK);
  LCD_DrawString(72U, 20U, "TOUCH CAL", ILI9341_YELLOW, ILI9341_BLACK,
                 &Font_16x8);
  LCD_DrawString(68U, 44U, detail, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);
  LCD_DrawString(18U, 210U, "PRESS MARKER CENTER", ILI9341_CYAN, ILI9341_BLACK,
                 &Font_16x8);

  LCD_DrawCircle(x, y, 12U, ILI9341_WHITE);
  LCD_DrawHLine((uint16_t)(x - 16U), y, 33U, ILI9341_RED);
  LCD_DrawVLine(x, (uint16_t)(y - 16U), 33U, ILI9341_RED);
}

static void GUI_DrawSystemInfoScreen(void) {
  char text[40];
  uint16_t y = 42U;

  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("SYSTEM INFO");
  GUI_DrawBackButton();

  snprintf(text, sizeof(text), "FW %s", FIRMWARE_VERSION);
  LCD_DrawString(10U, y, text, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);
  y += 22U;

  GUI_DrawSystemInfoStateRow();
  y += 22U;

  snprintf(text, sizeof(text), "PH SENSOR %s",
           (gSystemStatus.ph_sensor_ok != 0U) ? "OK" : "ERR");
  LCD_DrawString(10U, y, text, ILI9341_CYAN, ILI9341_BLACK, &Font_16x8);
  y += 22U;

  snprintf(text, sizeof(text), "EC SENSOR %s",
           (gSystemStatus.ec_sensor_ok != 0U) ? "OK" : "ERR");
  LCD_DrawString(10U, y, text, ILI9341_GREEN, ILI9341_BLACK, &Font_16x8);
  y += 22U;

  snprintf(text, sizeof(text), "EEPROM %s",
           (gSystemStatus.eeprom_ok != 0U) ? "OK" : "ERR");
  LCD_DrawString(10U, y, text, ILI9341_YELLOW, ILI9341_BLACK, &Font_16x8);
  y += 22U;

  snprintf(text, sizeof(text), "LCD %s  TOUCH %s",
           (gSystemStatus.lcd_ok != 0U) ? "OK" : "ERR",
           (gSystemStatus.touch_ok != 0U) ? "OK" : "ERR");
  LCD_DrawString(10U, y, text, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);

  GUI_DrawFooterButton(10U, "SETTINGS", ILI9341_DARKGRAY);
  GUI_DrawFooterButton(165U, "MAIN", ILI9341_NAVY);
}

static void GUI_DrawProgramsScreen(void) {
  char text[40];

  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("PROGRAM LIST");
  GUI_DrawBackButton();

  for (uint8_t i = 0U; i < VALVE_COUNT; i++) {
    irrigation_program_t program = {0};
    uint16_t x = (i % 2U == 0U) ? 10U : 165U;
    uint16_t y = 38U + (uint16_t)((i / 2U) * 38U);

    IRRIGATION_CTRL_GetProgram(i + 1U, &program);
    snprintf(text, sizeof(text), "P%u %s %04u", i + 1U,
             (program.enabled != 0U) ? "ON" : "OFF", program.start_hhmm);
    GUI_DrawButton(x, y, 145U, 30U, text,
                   (g_gui_state.selected_program == (i + 1U)) ? ILI9341_NAVY
                                                              : ILI9341_DARKGRAY);
  }

  GUI_DrawFooterButton(10U, "EDIT", ILI9341_DARKGREEN);
  GUI_DrawFooterButton(165U, "SETTINGS", ILI9341_NAVY);
}

static void GUI_DrawProgramEditScreen(void) {
  char title[24];
  char value_text[24];

  LCD_Clear(ILI9341_BLACK);
  snprintf(title, sizeof(title), "PROGRAM %u P%u", g_gui_state.selected_program,
           g_gui_state.program_edit_page + 1U);
  GUI_DrawHeader(title);
  GUI_DrawBackButton();

  if (g_gui_state.program_edit_page == 0U) {
    snprintf(value_text, sizeof(value_text), "%s",
             (g_gui_state.program_draft.enabled != 0U) ? "ON" : "OFF");
    GUI_DrawParcelAdjustRow("ENABLED", value_text, 38U);
    snprintf(value_text, sizeof(value_text), "%04u", g_gui_state.program_draft.start_hhmm);
    GUI_DrawParcelAdjustRow("START", value_text, 72U);
    snprintf(value_text, sizeof(value_text), "%04u", g_gui_state.program_draft.end_hhmm);
    GUI_DrawParcelAdjustRow("END", value_text, 106U);
    GUI_FormatValveMask(value_text, sizeof(value_text),
                        g_gui_state.program_draft.valve_mask);
    GUI_DrawParcelAdjustRow("MASK", value_text, 140U);
    snprintf(value_text, sizeof(value_text), "%u MIN",
             g_gui_state.program_draft.irrigation_min);
    GUI_DrawParcelAdjustRow("DUR", value_text, 174U);
  } else {
    snprintf(value_text, sizeof(value_text), "%u MIN",
             g_gui_state.program_draft.wait_min);
    GUI_DrawParcelAdjustRow("WAIT", value_text, 38U);
    snprintf(value_text, sizeof(value_text), "%u",
             g_gui_state.program_draft.repeat_count);
    GUI_DrawParcelAdjustRow("REPEAT", value_text, 72U);
    GUI_FormatDaysMask(value_text, sizeof(value_text),
                       g_gui_state.program_draft.days_mask);
    GUI_DrawParcelAdjustRow("DAYS", value_text, 106U);
    snprintf(value_text, sizeof(value_text), "%ld.%02ld",
             (long)(g_gui_state.program_draft.ph_set_x100 / 100),
             (long)labs(g_gui_state.program_draft.ph_set_x100 % 100));
    GUI_DrawParcelAdjustRow("PH", value_text, 140U);
    snprintf(value_text, sizeof(value_text), "%ld.%02ld",
             (long)(g_gui_state.program_draft.ec_set_x100 / 100),
             (long)labs(g_gui_state.program_draft.ec_set_x100 % 100));
    GUI_DrawParcelAdjustRow("EC", value_text, 174U);
  }

  GUI_DrawButton(10U, 208U, 90U, 24U, "APPLY", ILI9341_DARKGREEN);
  GUI_DrawButton(110U, 208U, 90U, 24U, "PAGE", ILI9341_DARKGRAY);
  GUI_DrawButton(210U, 208U, 100U, 24U, "LIST", ILI9341_NAVY);
}

static void GUI_DrawRTCSettingsScreen(void) {
  char text[24];

  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("RTC SETTINGS");
  GUI_DrawBackButton();

  snprintf(text, sizeof(text), "%02u:%02u", g_gui_state.rtc_time_draft.hours,
           g_gui_state.rtc_time_draft.minutes);
  GUI_DrawParcelAdjustRow("TIME", text, 48U);
  LCD_DrawString(208U, 58U, "H/M", ILI9341_WHITE, ILI9341_BLACK, &Font_8x5);

  snprintf(text, sizeof(text), "%02u/%02u", g_gui_state.rtc_date_draft.date,
           g_gui_state.rtc_date_draft.month);
  GUI_DrawParcelAdjustRow("DATE", text, 92U);
  LCD_DrawString(208U, 102U, "D/M", ILI9341_WHITE, ILI9341_BLACK, &Font_8x5);

  snprintf(text, sizeof(text), "YR %02u WD %u", g_gui_state.rtc_date_draft.year,
           g_gui_state.rtc_date_draft.weekday);
  LCD_DrawString(10U, 146U, text, ILI9341_CYAN, ILI9341_BLACK, &Font_16x8);
  LCD_DrawString(10U, 172U, "LEFT +/- HOUR DAY", ILI9341_WHITE, ILI9341_BLACK,
                 &Font_8x5);
  LCD_DrawString(10U, 184U, "RIGHT +/- MIN MONTH", ILI9341_WHITE, ILI9341_BLACK,
                 &Font_8x5);

  GUI_DrawFooterButton(10U, "APPLY", ILI9341_DARKGREEN);
  GUI_DrawFooterButton(165U, "SETTINGS", ILI9341_NAVY);
}

static void GUI_DrawAutoModeScreen(void) {
  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("AUTO MODE");
  GUI_DrawBackButton();

  GUI_DrawButton(10U, 38U, 300U, 28U, "OFF", 
                 (IRRIGATION_CTRL_GetAutoMode() == AUTO_MODE_OFF) ? ILI9341_NAVY : ILI9341_DARKGRAY);
  GUI_DrawButton(10U, 72U, 300U, 28U, "PH/EC ONLY",
                 (IRRIGATION_CTRL_GetAutoMode() == AUTO_MODE_PH_EC_ONLY) ? ILI9341_NAVY : ILI9341_DARKGRAY);
  GUI_DrawButton(10U, 106U, 300U, 28U, "PARCEL ONLY",
                 (IRRIGATION_CTRL_GetAutoMode() == AUTO_MODE_PARCEL_ONLY) ? ILI9341_NAVY : ILI9341_DARKGRAY);
  GUI_DrawButton(10U, 140U, 300U, 28U, "FULL AUTO",
                 (IRRIGATION_CTRL_GetAutoMode() == AUTO_MODE_FULL_AUTO) ? ILI9341_NAVY : ILI9341_DARKGRAY);
  GUI_DrawButton(10U, 174U, 300U, 28U, "SCHEDULED",
                 (IRRIGATION_CTRL_GetAutoMode() == AUTO_MODE_SCHEDULED) ? ILI9341_NAVY : ILI9341_DARKGRAY);
  GUI_DrawFooterButton(10U, "SETTINGS", ILI9341_DARKGRAY);
  GUI_DrawFooterButton(165U, "MAIN", ILI9341_NAVY);
}

static void GUI_DrawAlarmsScreen(void) {
  char text[40];
  irrigation_runtime_backup_t backup = {0};

  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("ALARMS");
  GUI_DrawBackButton();

  snprintf(text, sizeof(text), "ALARM %s", gSystemStatus.alarm_text);
  LCD_DrawString(10U, 42U, text, ILI9341_YELLOW, ILI9341_BLACK, &Font_16x8);
  snprintf(text, sizeof(text), "ERR %u", gSystemStatus.error_code);
  LCD_DrawString(10U, 68U, text, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);
  snprintf(text, sizeof(text), "PH %s  EC %s",
           (gSystemStatus.ph_sensor_ok != 0U) ? "OK" : "ERR",
           (gSystemStatus.ec_sensor_ok != 0U) ? "OK" : "ERR");
  LCD_DrawString(10U, 94U, text, ILI9341_CYAN, ILI9341_BLACK, &Font_16x8);
  snprintf(text, sizeof(text), "RTC %s  EEPROM %s",
           (gSystemStatus.rtc_ok != 0U) ? "OK" : "ERR",
           (gSystemStatus.eeprom_crc_ok != 0U) ? "OK" : "CRC");
  LCD_DrawString(10U, 120U, text, ILI9341_GREEN, ILI9341_BLACK, &Font_16x8);

  IRRIGATION_CTRL_GetRuntimeBackup(&backup);
  snprintf(text, sizeof(text), "PRG %u VALVE %u LEFT %u",
           backup.active_program_id, backup.active_valve_id, backup.remaining_sec);
  LCD_DrawString(10U, 146U, text, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);

  GUI_DrawFooterButton(10U, "SETTINGS", ILI9341_DARKGRAY);
  GUI_DrawFooterButton(165U, "MAIN", ILI9341_NAVY);
}

static void GUI_SetNotice(const char *text) {
  snprintf(g_gui_state.notice, sizeof(g_gui_state.notice), "%s", text);
}

static void GUI_DrawManualValveButton(uint8_t valve_id) {
  char label[20];
  uint16_t x;
  uint16_t y;
  uint8_t is_open;

  if (valve_id == 0U || valve_id > VALVE_COUNT) return;

  x = (valve_id % 2U == 1U) ? 10U : 165U;
  y = 42U + (uint16_t)(((valve_id - 1U) / 2U) * 39U);
  is_open = (VALVES_GetState(valve_id) == VALVE_STATE_OPEN);

  snprintf(label, sizeof(label), "V%u %s", valve_id,
           (is_open != 0U) ? "OPEN" : "CLOSED");
  GUI_DrawButton(x, y, 145U, 32U, label,
                 (is_open != 0U) ? ILI9341_DARKGREEN : ILI9341_DARKGRAY);
}

static void GUI_DrawSystemInfoStateRow(void) {
  char text[40];

  LCD_FillRect(10U, 64U, 300U, 16U, ILI9341_BLACK);
  snprintf(text, sizeof(text), "STATE %s",
           IRRIGATION_CTRL_GetStateName(IRRIGATION_CTRL_GetState()));
  LCD_DrawString(10U, 64U, text, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);
}

static const char *GUI_GetSystemHealthText(void) {
  if (gSystemStatus.error_code != 0U) {
    return "ALARM";
  }
  if (gSystemStatus.system_ready != 0U) {
    return "READY";
  }
  return "BOOT";
}

static const char *GUI_GetAutoModeName(auto_mode_t mode) {
  switch (mode) {
  case AUTO_MODE_OFF:
    return "OFF";
  case AUTO_MODE_PH_EC_ONLY:
    return "PH/EC";
  case AUTO_MODE_PARCEL_ONLY:
    return "PARCEL";
  case AUTO_MODE_FULL_AUTO:
    return "FULL";
  case AUTO_MODE_SCHEDULED:
    return "SCHED";
  default:
    return "NA";
  }
}

static void GUI_FormatValveMask(char *buffer, size_t buffer_size, uint8_t mask) {
  uint8_t first = 1U;
  size_t offset = 0U;

  if (buffer == NULL || buffer_size == 0U) return;

  if (mask == 0U) {
    snprintf(buffer, buffer_size, "NONE");
    return;
  }

  buffer[0] = '\0';
  for (uint8_t i = 0U; i < VALVE_COUNT; i++) {
    if ((mask & (1U << i)) == 0U) continue;
    offset += (size_t)snprintf(buffer + offset, buffer_size - offset, "%sV%u",
                               (first != 0U) ? "" : ",", i + 1U);
    first = 0U;
    if (offset >= buffer_size) {
      break;
    }
  }
}

static void GUI_FormatDaysMask(char *buffer, size_t buffer_size, uint8_t mask) {
  static const char *day_names[7] = {"M", "T", "W", "T", "F", "S", "S"};
  size_t offset = 0U;

  if (buffer == NULL || buffer_size == 0U) return;

  if (mask == 0U) {
    snprintf(buffer, buffer_size, "NONE");
    return;
  }

  buffer[0] = '\0';
  for (uint8_t i = 0U; i < 7U; i++) {
    if ((mask & (1U << i)) == 0U) continue;
    offset += (size_t)snprintf(buffer + offset, buffer_size - offset, "%s",
                               day_names[i]);
    if (offset >= buffer_size) {
      break;
    }
  }
}

static uint16_t GUI_AdjustHHMM(uint16_t hhmm, int16_t delta_minutes) {
  int32_t hours = (int32_t)(hhmm / 100U);
  int32_t minutes = (int32_t)(hhmm % 100U);
  int32_t minute_of_day = (hours * 60L) + minutes + delta_minutes;

  while (minute_of_day < 0L) {
    minute_of_day += 1440L;
  }
  minute_of_day %= 1440L;

  hours = minute_of_day / 60L;
  minutes = minute_of_day % 60L;
  return (uint16_t)((hours * 100L) + minutes);
}

static uint8_t GUI_PointInRect(const touch_point_t *point, uint16_t x,
                               uint16_t y, uint16_t w, uint16_t h) {
  if (point == NULL) return 0U;

  return (point->x >= (int16_t)x && point->x < (int16_t)(x + w) &&
          point->y >= (int16_t)y && point->y < (int16_t)(y + h));
}

static void GUI_HandleMainTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, 10U, 195U, 96U, 34U) != 0U) {
    if (IRRIGATION_CTRL_IsRunning() != 0U) {
      IRRIGATION_CTRL_Stop();
    } else {
      IRRIGATION_CTRL_Start();
    }
    return;
  }

  if (GUI_PointInRect(point, 112U, 195U, 96U, 34U) != 0U) {
    GUI_NavigateTo(SCREEN_MANUAL);
    return;
  }

  if (GUI_PointInRect(point, 214U, 195U, 96U, 34U) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
  }
}

static void GUI_HandleManualTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, 4U, 64U, 22U) != 0U) {
    GUI_NavigateTo(SCREEN_MAIN);
    return;
  }

  if (GUI_PointInRect(point, 10U, 202U, 145U, 28U) != 0U) {
    IRRIGATION_CTRL_Stop();
    VALVES_CloseAll();
    return;
  }

  if (GUI_PointInRect(point, 165U, 202U, 145U, 28U) != 0U) {
    GUI_NavigateTo(SCREEN_MAIN);
    return;
  }

  for (uint8_t valve_id = 1U; valve_id <= VALVE_COUNT; valve_id++) {
    uint16_t x = (valve_id % 2U == 1U) ? 10U : 165U;
    uint16_t y = 42U + (uint16_t)(((valve_id - 1U) / 2U) * 39U);

    if (GUI_PointInRect(point, x, y, 145U, 32U) == 0U) continue;

    if (IRRIGATION_CTRL_IsRunning() != 0U) {
      IRRIGATION_CTRL_Stop();
    }

    if (VALVES_GetState(valve_id) == VALVE_STATE_OPEN) {
      VALVES_ManualClose(valve_id);
    } else {
      VALVES_ManualOpen(valve_id);
    }
    return;
  }
}

static void GUI_HandleSettingsTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, 4U, 64U, 22U) != 0U) {
    GUI_NavigateTo(SCREEN_MAIN);
    return;
  }

  if (GUI_PointInRect(point, 10U, 38U, 145U, 30U) != 0U) {
    GUI_NavigateTo(SCREEN_PH_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 165U, 38U, 145U, 30U) != 0U) {
    GUI_NavigateTo(SCREEN_EC_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 10U, 74U, 145U, 30U) != 0U) {
    GUI_NavigateTo(SCREEN_PARCEL_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 165U, 74U, 145U, 30U) != 0U) {
    GUI_NavigateTo(SCREEN_CALIBRATION);
    return;
  }

  if (GUI_PointInRect(point, 10U, 110U, 145U, 30U) != 0U) {
    GUI_NavigateTo(SCREEN_PROGRAMS);
    return;
  }

  if (GUI_PointInRect(point, 165U, 110U, 145U, 30U) != 0U) {
    GUI_NavigateTo(SCREEN_RTC_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 10U, 146U, 145U, 30U) != 0U) {
    GUI_NavigateTo(SCREEN_AUTO_MODE);
    return;
  }

  if (GUI_PointInRect(point, 165U, 146U, 145U, 30U) != 0U) {
    GUI_NavigateTo(SCREEN_ALARMS);
    return;
  }

  if (GUI_PointInRect(point, 10U, 202U, 145U, 28U) != 0U) {
    GUI_NavigateTo(SCREEN_SYSTEM_INFO);
    return;
  }

  if (GUI_PointInRect(point, 165U, 202U, 145U, 28U) != 0U) {
    GUI_NavigateTo(SCREEN_MAIN);
  }
}

static void GUI_HandlePHSettingsTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, 4U, 64U, 22U) != 0U ||
      GUI_PointInRect(point, 165U, 202U, 145U, 28U) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 215U, 46U, 40U, 30U) != 0U) {
    g_gui_state.ph_draft.target -= 0.1f;
  } else if (GUI_PointInRect(point, 265U, 46U, 40U, 30U) != 0U) {
    g_gui_state.ph_draft.target += 0.1f;
  } else if (GUI_PointInRect(point, 215U, 88U, 40U, 30U) != 0U) {
    g_gui_state.ph_draft.min_limit -= 0.1f;
  } else if (GUI_PointInRect(point, 265U, 88U, 40U, 30U) != 0U) {
    g_gui_state.ph_draft.min_limit += 0.1f;
  } else if (GUI_PointInRect(point, 215U, 130U, 40U, 30U) != 0U) {
    g_gui_state.ph_draft.max_limit -= 0.1f;
  } else if (GUI_PointInRect(point, 265U, 130U, 40U, 30U) != 0U) {
    g_gui_state.ph_draft.max_limit += 0.1f;
  } else if (GUI_PointInRect(point, 10U, 202U, 145U, 28U) != 0U) {
    IRRIGATION_CTRL_SetPHParams(g_gui_state.ph_draft.target,
                                g_gui_state.ph_draft.min_limit,
                                g_gui_state.ph_draft.max_limit,
                                g_gui_state.ph_draft.hysteresis);
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  GUI_DrawPHSettingsScreen();
}

static void GUI_HandleECSettingsTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, 4U, 64U, 22U) != 0U ||
      GUI_PointInRect(point, 165U, 202U, 145U, 28U) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 215U, 46U, 40U, 30U) != 0U) {
    g_gui_state.ec_draft.target -= 0.1f;
  } else if (GUI_PointInRect(point, 265U, 46U, 40U, 30U) != 0U) {
    g_gui_state.ec_draft.target += 0.1f;
  } else if (GUI_PointInRect(point, 215U, 88U, 40U, 30U) != 0U) {
    g_gui_state.ec_draft.min_limit -= 0.1f;
  } else if (GUI_PointInRect(point, 265U, 88U, 40U, 30U) != 0U) {
    g_gui_state.ec_draft.min_limit += 0.1f;
  } else if (GUI_PointInRect(point, 215U, 130U, 40U, 30U) != 0U) {
    g_gui_state.ec_draft.max_limit -= 0.1f;
  } else if (GUI_PointInRect(point, 265U, 130U, 40U, 30U) != 0U) {
    g_gui_state.ec_draft.max_limit += 0.1f;
  } else if (GUI_PointInRect(point, 10U, 202U, 145U, 28U) != 0U) {
    IRRIGATION_CTRL_SetECParams(g_gui_state.ec_draft.target,
                                g_gui_state.ec_draft.min_limit,
                                g_gui_state.ec_draft.max_limit,
                                g_gui_state.ec_draft.hysteresis);
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  GUI_DrawECSettingsScreen();
}

static void GUI_HandleParcelSettingsTouch(const touch_point_t *point) {
  uint8_t reload_selected_parcel = 0U;

  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, 4U, 64U, 22U) != 0U ||
      GUI_PointInRect(point, 165U, 202U, 145U, 28U) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 215U, 46U, 40U, 30U) != 0U) {
    if (g_gui_state.selected_parcel > 1U) {
      g_gui_state.selected_parcel--;
      reload_selected_parcel = 1U;
    }
  } else if (GUI_PointInRect(point, 265U, 46U, 40U, 30U) != 0U) {
    if (g_gui_state.selected_parcel < VALVE_COUNT) {
      g_gui_state.selected_parcel++;
      reload_selected_parcel = 1U;
    }
  } else if (GUI_PointInRect(point, 215U, 88U, 40U, 30U) != 0U) {
    if (g_gui_state.parcel_duration_sec > 30U) {
      g_gui_state.parcel_duration_sec -= 30U;
    }
  } else if (GUI_PointInRect(point, 265U, 88U, 40U, 30U) != 0U) {
    g_gui_state.parcel_duration_sec += 30U;
  } else if (GUI_PointInRect(point, 215U, 130U, 40U, 30U) != 0U ||
             GUI_PointInRect(point, 265U, 130U, 40U, 30U) != 0U) {
    g_gui_state.parcel_enabled =
        (g_gui_state.parcel_enabled == 0U) ? 1U : 0U;
  } else if (GUI_PointInRect(point, 10U, 202U, 145U, 28U) != 0U) {
    PARCELS_SetDuration(g_gui_state.selected_parcel,
                        g_gui_state.parcel_duration_sec);
    PARCELS_SetEnabled(g_gui_state.selected_parcel,
                       g_gui_state.parcel_enabled);
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (reload_selected_parcel != 0U) {
    GUI_LoadScreenState(SCREEN_PARCEL_SETTINGS);
  }
  GUI_DrawParcelSettingsScreen();
}

static void GUI_HandleCalibrationTouch(const touch_point_t *point) {
  uint8_t step = 0U;

  if (TOUCH_IsCalibrating() != 0U) {
    if (TOUCH_CalibrationProcess((uint16_t *)&point->raw_x,
                                 (uint16_t *)&point->raw_y, &step) != 0U) {
      GUI_SetNotice("TOUCH CAL OK");
    } else {
      GUI_SetNotice("NEXT TARGET");
    }
    GUI_Redraw();
    return;
  }

  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, 4U, 64U, 22U) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 10U, 48U, 145U, 42U) != 0U) {
    TOUCH_CalibrationStart();
    GUI_SetNotice("TOUCH CAL RUN");
    GUI_Redraw();
    return;
  }

  if (GUI_PointInRect(point, 165U, 48U, 145U, 42U) != 0U) {
    GUI_SetNotice("PH CAL ENTRY");
    GUI_DrawCalibrationScreen();
    return;
  }

  if (GUI_PointInRect(point, 10U, 100U, 145U, 42U) != 0U) {
    GUI_SetNotice("EC CAL ENTRY");
    GUI_DrawCalibrationScreen();
    return;
  }

  if (GUI_PointInRect(point, 165U, 100U, 145U, 42U) != 0U) {
    GUI_NavigateTo(SCREEN_MAIN);
  }
}

static void GUI_HandleSystemInfoTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, 4U, 64U, 22U) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 10U, 202U, 145U, 28U) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 165U, 202U, 145U, 28U) != 0U) {
    GUI_NavigateTo(SCREEN_MAIN);
  }
}

static void GUI_HandleProgramsTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, 4U, 64U, 22U) != 0U ||
      GUI_PointInRect(point, 165U, 202U, 145U, 28U) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 10U, 202U, 145U, 28U) != 0U) {
    GUI_NavigateTo(SCREEN_PROGRAM_EDIT);
    return;
  }

  for (uint8_t i = 0U; i < VALVE_COUNT; i++) {
    uint16_t x = (i % 2U == 0U) ? 10U : 165U;
    uint16_t y = 38U + (uint16_t)((i / 2U) * 38U);

    if (GUI_PointInRect(point, x, y, 145U, 30U) == 0U) continue;

    g_gui_state.selected_program = i + 1U;
    GUI_DrawProgramsScreen();
    return;
  }
}

static void GUI_HandleProgramEditTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, 4U, 64U, 22U) != 0U ||
      GUI_PointInRect(point, 210U, 208U, 100U, 24U) != 0U) {
    GUI_NavigateTo(SCREEN_PROGRAMS);
    return;
  }

  if (GUI_PointInRect(point, 110U, 208U, 90U, 24U) != 0U) {
    g_gui_state.program_edit_page ^= 1U;
    GUI_DrawProgramEditScreen();
    return;
  }

  if (GUI_PointInRect(point, 10U, 208U, 90U, 24U) != 0U) {
    IRRIGATION_CTRL_SetProgram(g_gui_state.selected_program,
                               &g_gui_state.program_draft);
    GUI_NavigateTo(SCREEN_PROGRAMS);
    return;
  }

  for (uint8_t row = 0U; row < 5U; row++) {
    uint16_t y = (g_gui_state.program_edit_page == 0U) ?
                     (uint16_t)(38U + (row * 34U)) :
                     (uint16_t)(38U + (row * 34U));
    uint8_t minus_pressed = GUI_PointInRect(point, 215U, y, 40U, 30U);
    uint8_t plus_pressed = GUI_PointInRect(point, 265U, y, 40U, 30U);

    if (minus_pressed == 0U && plus_pressed == 0U) continue;

    if (g_gui_state.program_edit_page == 0U) {
      switch (row) {
      case 0U:
        g_gui_state.program_draft.enabled ^= 1U;
        break;
      case 1U:
        g_gui_state.program_draft.start_hhmm =
            GUI_AdjustHHMM(g_gui_state.program_draft.start_hhmm,
                           (minus_pressed != 0U) ? -15 : 15);
        break;
      case 2U:
        g_gui_state.program_draft.end_hhmm =
            GUI_AdjustHHMM(g_gui_state.program_draft.end_hhmm,
                           (minus_pressed != 0U) ? -15 : 15);
        break;
      case 3U:
        if (minus_pressed != 0U) {
          g_gui_state.program_draft.valve_mask >>= 1U;
        } else {
          g_gui_state.program_draft.valve_mask =
              (uint8_t)((g_gui_state.program_draft.valve_mask << 1U) | 1U);
        }
        if (g_gui_state.program_draft.valve_mask == 0U) {
          g_gui_state.program_draft.valve_mask = 1U;
        }
        break;
      case 4U:
        if (minus_pressed != 0U && g_gui_state.program_draft.irrigation_min > 1U) {
          g_gui_state.program_draft.irrigation_min--;
        } else if (plus_pressed != 0U && g_gui_state.program_draft.irrigation_min < 999U) {
          g_gui_state.program_draft.irrigation_min++;
        }
        break;
      default:
        break;
      }
    } else {
      switch (row) {
      case 0U:
        if (minus_pressed != 0U && g_gui_state.program_draft.wait_min > 0U) {
          g_gui_state.program_draft.wait_min--;
        } else if (plus_pressed != 0U && g_gui_state.program_draft.wait_min < 999U) {
          g_gui_state.program_draft.wait_min++;
        }
        break;
      case 1U:
        if (minus_pressed != 0U && g_gui_state.program_draft.repeat_count > 1U) {
          g_gui_state.program_draft.repeat_count--;
        } else if (plus_pressed != 0U && g_gui_state.program_draft.repeat_count < 99U) {
          g_gui_state.program_draft.repeat_count++;
        }
        break;
      case 2U:
        if (minus_pressed != 0U) {
          g_gui_state.program_draft.days_mask >>= 1U;
        } else {
          g_gui_state.program_draft.days_mask =
              (uint8_t)((g_gui_state.program_draft.days_mask << 1U) | 1U);
        }
        if ((g_gui_state.program_draft.days_mask & 0x7FU) == 0U) {
          g_gui_state.program_draft.days_mask = 0x01U;
        }
        g_gui_state.program_draft.days_mask &= 0x7FU;
        break;
      case 3U:
        g_gui_state.program_draft.ph_set_x100 += (minus_pressed != 0U) ? -10 : 10;
        break;
      case 4U:
        g_gui_state.program_draft.ec_set_x100 += (minus_pressed != 0U) ? -10 : 10;
        break;
      default:
        break;
      }
    }

    GUI_DrawProgramEditScreen();
    return;
  }
}

static void GUI_HandleRTCSettingsTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, 4U, 64U, 22U) != 0U ||
      GUI_PointInRect(point, 165U, 202U, 145U, 28U) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 10U, 202U, 145U, 28U) != 0U) {
    RTC_SetTime(&g_gui_state.rtc_time_draft);
    RTC_SetDate(&g_gui_state.rtc_date_draft);
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 215U, 48U, 40U, 30U) != 0U) {
    uint16_t hhmm = (uint16_t)((g_gui_state.rtc_time_draft.hours * 100U) +
                               g_gui_state.rtc_time_draft.minutes);
    hhmm = GUI_AdjustHHMM(hhmm, -60);
    g_gui_state.rtc_time_draft.hours = (uint8_t)(hhmm / 100U);
    g_gui_state.rtc_time_draft.minutes = (uint8_t)(hhmm % 100U);
  } else if (GUI_PointInRect(point, 265U, 48U, 40U, 30U) != 0U) {
    uint16_t hhmm = (uint16_t)((g_gui_state.rtc_time_draft.hours * 100U) +
                               g_gui_state.rtc_time_draft.minutes);
    hhmm = GUI_AdjustHHMM(hhmm, 60);
    g_gui_state.rtc_time_draft.hours = (uint8_t)(hhmm / 100U);
    g_gui_state.rtc_time_draft.minutes = (uint8_t)(hhmm % 100U);
  } else if (GUI_PointInRect(point, 215U, 92U, 40U, 30U) != 0U) {
    g_gui_state.rtc_date_draft.date =
        (g_gui_state.rtc_date_draft.date > 1U) ? (g_gui_state.rtc_date_draft.date - 1U) : 31U;
  } else if (GUI_PointInRect(point, 265U, 92U, 40U, 30U) != 0U) {
    g_gui_state.rtc_date_draft.month =
        (uint8_t)((g_gui_state.rtc_date_draft.month % 12U) + 1U);
  } else {
    return;
  }

  GUI_DrawRTCSettingsScreen();
}

static void GUI_HandleAutoModeTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, 4U, 64U, 22U) != 0U ||
      GUI_PointInRect(point, 10U, 202U, 145U, 28U) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 165U, 202U, 145U, 28U) != 0U) {
    GUI_NavigateTo(SCREEN_MAIN);
    return;
  }

  if (GUI_PointInRect(point, 10U, 38U, 300U, 28U) != 0U) {
    IRRIGATION_CTRL_SetAutoMode(AUTO_MODE_OFF);
  } else if (GUI_PointInRect(point, 10U, 72U, 300U, 28U) != 0U) {
    IRRIGATION_CTRL_SetAutoMode(AUTO_MODE_PH_EC_ONLY);
  } else if (GUI_PointInRect(point, 10U, 106U, 300U, 28U) != 0U) {
    IRRIGATION_CTRL_SetAutoMode(AUTO_MODE_PARCEL_ONLY);
  } else if (GUI_PointInRect(point, 10U, 140U, 300U, 28U) != 0U) {
    IRRIGATION_CTRL_SetAutoMode(AUTO_MODE_FULL_AUTO);
  } else if (GUI_PointInRect(point, 10U, 174U, 300U, 28U) != 0U) {
    IRRIGATION_CTRL_SetAutoMode(AUTO_MODE_SCHEDULED);
  } else {
    return;
  }

  GUI_DrawAutoModeScreen();
}

static void GUI_HandleAlarmsTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, 4U, 64U, 22U) != 0U ||
      GUI_PointInRect(point, 10U, 202U, 145U, 28U) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 165U, 202U, 145U, 28U) != 0U) {
    GUI_NavigateTo(SCREEN_MAIN);
  }
}
