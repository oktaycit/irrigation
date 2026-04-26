/**
 ******************************************************************************
 * @file           : gui.c
 * @brief          : Grafiksel Kullanıcı Arayüzü (GUI) Driver Implementation
 ******************************************************************************
 */

#include "main.h"
#include "gui_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GUI_HEADER_HEIGHT 24U
#define GUI_TOUCH_DEBOUNCE_MS 200U  /* Increased debounce for reliable touch */
#define GUI_BACK_BUTTON_W 64U
#define GUI_BACK_BUTTON_H 36U
#define GUI_BACK_BUTTON_Y 0U
#define GUI_FOOTER_BUTTON_Y 202U
#define GUI_FOOTER_BUTTON_W 145U
#define GUI_FOOTER_BUTTON_H 36U
#define GUI_LIST_PAGE_SIZE 8U
#define GUI_PROGRAM_EDIT_ROW_Y0 32U
#define GUI_PROGRAM_EDIT_ROW_STEP 27U
#define GUI_PROGRAM_EDIT_ROW_COUNT 5U
#define GUI_PROGRAM_EDIT_PAGE_COUNT 4U
#define GUI_PROGRAM_EDIT_FOOTER_Y 202U
#define GUI_PROGRAM_EDIT_BUTTON_W 96U
#define GUI_PROGRAM_EDIT_BUTTON_H 36U
#define GUI_PROGRAM_EDIT_BUTTON_GAP 6U

typedef struct {
  ph_control_params_t ph_draft;
  ec_control_params_t ec_draft;
  uint8_t selected_parcel;
  uint32_t parcel_duration_sec;
  uint8_t parcel_enabled;
  uint8_t selected_program;
  uint8_t manual_page;
  uint8_t programs_page;
  uint8_t program_edit_page;
  uint8_t params_page;
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
static control_state_t g_gui_last_main_state = CTRL_STATE_IDLE;
static char g_gui_last_health_text[16] = {0};
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
static void GUI_DrawDosingScreen(void);
static void GUI_DrawParametersScreen(void);
static void GUI_DrawBackButton(void);
static void GUI_DrawFooterButton(uint16_t x, const char *text,
                                 lcd_color_t color);
static void GUI_FormatValueText(char *buffer, size_t buffer_size, float value,
                                const char *unit);
static void GUI_FormatFixed2(char *buffer, size_t buffer_size, float value);
static void GUI_DrawValueTextArea(uint16_t x, uint16_t y, uint16_t w,
                                  uint16_t h, const char *value_text,
                                  lcd_color_t bg);
static void GUI_DrawTextClipped(uint16_t x, uint16_t y, uint16_t w,
                                const char *text, lcd_color_t fg,
                                lcd_color_t bg, const lcd_font_t *font);
static void GUI_DrawTextWrapped2(uint16_t x, uint16_t y, uint16_t w,
                                 const char *text, lcd_color_t fg,
                                 lcd_color_t bg, const lcd_font_t *font);
static void GUI_FormatMMSS(char *buffer, size_t buffer_size,
                           uint32_t seconds);
static uint16_t GUI_AdjustHHMM(uint16_t hhmm, int16_t delta_minutes);
#if BOARD_SENSOR_DEMO_MODE
static uint8_t GUI_GetDemoValueIndex(void);
static float GUI_GetDemoPHValue(void);
static float GUI_GetDemoECValue(void);
#endif
static void GUI_DrawAdjustRow(uint16_t y, const char *label, float value,
                              const char *unit);
static void GUI_DrawUIntAdjustRow(uint16_t y, const char *label, uint32_t value,
                                  const char *unit);
static void GUI_DrawParcelAdjustRow(const char *label, const char *value,
                                    uint16_t y);
static void GUI_DrawMainModeLine(void);
static void GUI_FormatMainDateTime(char *buffer, size_t buffer_size);
static void GUI_DrawMainParcelLine(uint8_t current_parcel,
                                   uint32_t remaining_sec);
static uint32_t GUI_GetMainProgressDurationSec(uint8_t parcel_id,
                                               uint32_t remaining_sec);
static const char *GUI_GetValveLabel(uint8_t valve_id);
static void GUI_DrawManualValveButton(uint8_t valve_id);
static void GUI_DrawSystemInfoStateRow(void);
static void GUI_SetNotice(const char *text);
static uint8_t GUI_GetPageCount(uint8_t item_count, uint8_t page_size);
static uint8_t GUI_GetPageStartIndex(uint8_t page, uint8_t page_size);
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
static void GUI_HandleDosingTouch(const touch_point_t *point);
static void GUI_HandleParametersTouch(const touch_point_t *point);
static void GUI_RequestRedraw(void);
static GPIO_PinState GUI_ReadDosingGPIO(uint8_t valve_id);
static void GUI_ToggleDosingGPIO(uint8_t valve_id);

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
    LOW_POWER_UpdateActivity();
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
    const char *health_text = GUI_GetSystemHealthText();
    control_state_t current_state = IRRIGATION_CTRL_GetState();
    uint8_t state_changed =
        (current_state != g_gui_last_main_state) ? 1U : 0U;

    if (remaining_sec != g_gui_last_main_remaining_sec ||
        state_changed != 0U) {
      g_gui_last_main_remaining_sec = remaining_sec;
      g_gui_last_main_state = current_state;
      GUI_DrawMainParcelLine(IRRIGATION_CTRL_GetCurrentParcelId(), remaining_sec);
      GUI_DrawMainProgress(IRRIGATION_CTRL_GetCurrentParcelId(), remaining_sec);
    }

    if (state_changed != 0U ||
        strcmp(health_text, g_gui_last_health_text) != 0) {
      g_gui_last_main_state = current_state;
      snprintf(g_gui_last_health_text, sizeof(g_gui_last_health_text), "%s",
               health_text);
      GUI_DrawStatusbar(IRRIGATION_CTRL_GetStateName(current_state),
                        health_text);
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
  g_gui_last_main_state = CTRL_STATE_IDLE;
  memset(g_gui_last_health_text, 0, sizeof(g_gui_last_health_text));
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
#if BOARD_SENSOR_DEMO_MODE
  float ph_value = GUI_GetDemoPHValue();
  float ec_value = GUI_GetDemoECValue();
#else
  float ph_value = IRRIGATION_CTRL_GetPH();
  float ec_value = IRRIGATION_CTRL_GetEC();
#endif

  LCD_Clear(LAYOUT_COLOR_BG);

#if BOARD_SENSOR_DEMO_MODE
  GUI_DrawStatusbar(IRRIGATION_CTRL_GetStateName(IRRIGATION_CTRL_GetState()),
                    "DEMO");
#else
  GUI_DrawStatusbar(IRRIGATION_CTRL_GetStateName(IRRIGATION_CTRL_GetState()),
                    GUI_GetSystemHealthText());
  g_gui_last_main_state = IRRIGATION_CTRL_GetState();
  snprintf(g_gui_last_health_text, sizeof(g_gui_last_health_text), "%s",
           GUI_GetSystemHealthText());
#endif

  /* Draw pH and EC value boxes */
  GUI_DrawValueBox(LAYOUT_VALUE_BOX_X1, LAYOUT_VALUE_BOX_Y,
                   LAYOUT_VALUE_BOX_WIDTH, LAYOUT_VALUE_BOX_HEIGHT,
                   "PH", ph_value, "", LAYOUT_COLOR_PH, LAYOUT_COLOR_BG);
  GUI_DrawValueBox(LAYOUT_VALUE_BOX_X2, LAYOUT_VALUE_BOX_Y,
                   LAYOUT_VALUE_BOX_WIDTH, LAYOUT_VALUE_BOX_HEIGHT,
                   "EC", ec_value, "MS", LAYOUT_COLOR_EC, LAYOUT_COLOR_BG);

  /* Draw mode and parcel information lines */
  GUI_DrawMainModeLine();
  GUI_DrawMainParcelLine(current_parcel, remaining_sec);

  /* Draw progress bar and action buttons */
  GUI_DrawMainProgress(current_parcel, remaining_sec);
  GUI_DrawMainActions();

  /* Draw separator line */
  LCD_DrawHLine(0U, LAYOUT_ACTION_BUTTONS_Y - 2U,
                LCD_GetDisplayWidth(), ILI9341_DARKGRAY);
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
  case SCREEN_DOSING:
    GUI_HandleDosingTouch(point);
    break;
  case SCREEN_PARAMETERS:
    GUI_HandleParametersTouch(point);
    break;
  default:
    break;
  }
}

/**
 * @brief  Draw common header
 */
void GUI_DrawHeader(const char *title) {
  LCD_FillRect(0U, LAYOUT_HEADER_Y, LCD_GetDisplayWidth(),
               LAYOUT_HEADER_HEIGHT, LAYOUT_COLOR_PRIMARY_DARK);
  LCD_DrawString(10U, LAYOUT_HEADER_Y + 4U, title, LAYOUT_COLOR_TEXT,
                 LAYOUT_COLOR_PRIMARY_DARK, LAYOUT_FONT_NORMAL);
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
  case SCREEN_DOSING:
    GUI_DrawDosingScreen();
    break;
  case SCREEN_PARAMETERS:
    GUI_DrawParametersScreen();
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
    g_gui_last_main_state = IRRIGATION_CTRL_GetState();
    snprintf(g_gui_last_health_text, sizeof(g_gui_last_health_text), "%s",
             GUI_GetSystemHealthText());
  }
}

void GUI_DrawValueBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                      const char *label, float value, const char *unit,
                      lcd_color_t fg, lcd_color_t bg) {
  char value_text[32];
  char *cache = NULL;

  if ((x == LAYOUT_VALUE_BOX_X1) && (y == LAYOUT_VALUE_BOX_Y)) {
    cache = g_gui_last_ph_text;
  } else if ((x == LAYOUT_VALUE_BOX_X2) && (y == LAYOUT_VALUE_BOX_Y)) {
    cache = g_gui_last_ec_text;
  }

  LCD_FillRect(x, y, w, h, bg);
  LCD_DrawRect(x, y, w, h, fg);
  LCD_DrawString(x + 8U, y + 4U, label, fg, bg, LAYOUT_FONT_NORMAL);
  GUI_FormatValueText(value_text, sizeof(value_text), value, unit);
  GUI_DrawValueTextArea(x, y, w, h, value_text, bg);

  if (cache != NULL) {
    snprintf(cache, 32U, "%s", value_text);
  }
}

void GUI_DrawStatusbar(const char *left_text, const char *right_text) {
  uint16_t screen_width = LCD_GetDisplayWidth();
  uint16_t right_x = 170U;

  LCD_FillRect(0U, LAYOUT_STATUSBAR_Y, screen_width, LAYOUT_STATUSBAR_HEIGHT,
               LAYOUT_COLOR_BG_LIGHT);
  LCD_DrawString(8U, LAYOUT_STATUSBAR_Y + 4U, left_text, LAYOUT_COLOR_TEXT,
                 LAYOUT_COLOR_BG_LIGHT, LAYOUT_FONT_NORMAL);

  if ((right_text != NULL) && (strlen(right_text) > 0U)) {
    uint16_t estimated_width = (uint16_t)(strlen(right_text) * Font_16x8.width);
    if (estimated_width + 10U < screen_width) {
      right_x = screen_width - estimated_width - 8U;
    }
    LCD_DrawString(right_x, LAYOUT_STATUSBAR_Y + 4U, right_text,
                   LAYOUT_COLOR_ACCENT, LAYOUT_COLOR_BG_LIGHT, LAYOUT_FONT_NORMAL);
  }
}

void GUI_UpdatePHValue(float ph) {
  char value_text[32];

  if (h_gui.is_initialized == 0U) return;
  GUI_FormatValueText(value_text, sizeof(value_text), ph, "");
  if (strcmp(value_text, g_gui_last_ph_text) == 0) return;

  if (h_gui.current_screen == SCREEN_MAIN && g_gui_dirty == 0U) {
    GUI_DrawValueTextArea(LAYOUT_VALUE_BOX_X1, LAYOUT_VALUE_BOX_Y,
                          LAYOUT_VALUE_BOX_WIDTH, LAYOUT_VALUE_BOX_HEIGHT,
                          value_text, LAYOUT_COLOR_BG);
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
    GUI_DrawValueTextArea(LAYOUT_VALUE_BOX_X2, LAYOUT_VALUE_BOX_Y,
                          LAYOUT_VALUE_BOX_WIDTH, LAYOUT_VALUE_BOX_HEIGHT,
                          value_text, LAYOUT_COLOR_BG);
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
  uint16_t value_y = y + 24U;
  uint16_t value_h = Font_16x8.height;

  (void)h;
  LCD_FillRect(x + 8U, value_y, w - 16U, value_h, bg);
  LCD_DrawString(x + 8U, value_y, value_text, ILI9341_WHITE, bg, &Font_16x8);
}

static void GUI_DrawTextClipped(uint16_t x, uint16_t y, uint16_t w,
                                const char *text, lcd_color_t fg,
                                lcd_color_t bg, const lcd_font_t *font) {
  char line[GUI_MAX_TEXT_LEN];
  size_t max_chars;
  size_t text_len;

  if (text == NULL || font == NULL || font->width == 0U || w == 0U) {
    return;
  }

  max_chars = w / font->width;
  if (max_chars == 0U) {
    return;
  }
  if (max_chars >= sizeof(line)) {
    max_chars = sizeof(line) - 1U;
  }

  text_len = strlen(text);
  if (text_len > max_chars) {
    memcpy(line, text, max_chars);
    line[max_chars] = '\0';
  } else {
    snprintf(line, sizeof(line), "%s", text);
  }

  LCD_FillRect(x, y, w, font->height, bg);
  LCD_DrawString(x, y, line, fg, bg, font);
}

static void GUI_DrawTextWrapped2(uint16_t x, uint16_t y, uint16_t w,
                                 const char *text, lcd_color_t fg,
                                 lcd_color_t bg, const lcd_font_t *font) {
  char first[GUI_MAX_TEXT_LEN] = {0};
  char second[GUI_MAX_TEXT_LEN] = {0};
  size_t max_chars;
  size_t split = 0U;
  size_t text_len;

  if (text == NULL || font == NULL || font->width == 0U || w == 0U) {
    return;
  }

  max_chars = w / font->width;
  if (max_chars == 0U) {
    return;
  }
  if (max_chars >= sizeof(first)) {
    max_chars = sizeof(first) - 1U;
  }

  text_len = strlen(text);
  if (text_len <= max_chars) {
    GUI_DrawTextClipped(x, y, w, text, fg, bg, font);
    LCD_FillRect(x, (uint16_t)(y + font->height + 2U), w, font->height, bg);
    return;
  }

  split = max_chars;
  while (split > 0U && text[split] != ' ') {
    split--;
  }
  if (split == 0U) {
    split = max_chars;
  }

  memcpy(first, text, split);
  first[split] = '\0';
  while (text[split] == ' ') {
    split++;
  }

  snprintf(second, sizeof(second), "%s", &text[split]);
  GUI_DrawTextClipped(x, y, w, first, fg, bg, font);
  GUI_DrawTextClipped(x, (uint16_t)(y + font->height + 2U), w, second, fg, bg,
                      font);
}

static void GUI_FormatMMSS(char *buffer, size_t buffer_size,
                           uint32_t seconds) {
  if (buffer == NULL || buffer_size == 0U) {
    return;
  }

  snprintf(buffer, buffer_size, "%02lu:%02lu",
           (unsigned long)(seconds / 60U), (unsigned long)(seconds % 60U));
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
  case SCREEN_PARAMETERS:
    IRRIGATION_CTRL_GetPHParams(&g_gui_state.ph_draft);
    IRRIGATION_CTRL_GetECParams(&g_gui_state.ec_draft);
    if (g_gui_state.params_page > 1U) {
      g_gui_state.params_page = 0U;
    }
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
  case SCREEN_ALARMS:
    GUI_SetNotice(IRRIGATION_CTRL_GetAlarmActionText());
    break;
  case SCREEN_PROGRAMS:
    if (g_gui_state.selected_program == 0U ||
        g_gui_state.selected_program > IRRIGATION_PROGRAM_COUNT) {
      g_gui_state.selected_program = 1U;
    }
    g_gui_state.programs_page =
        (uint8_t)((g_gui_state.selected_program - 1U) / GUI_LIST_PAGE_SIZE);
    break;
  case SCREEN_MANUAL:
    g_gui_state.manual_page = 0U;
    break;
  case SCREEN_PROGRAM_EDIT:
    if (g_gui_state.selected_program == 0U ||
        g_gui_state.selected_program > IRRIGATION_PROGRAM_COUNT) {
      g_gui_state.selected_program = 1U;
    }
    g_gui_state.program_edit_page = 0U;
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
  GUI_DrawButton(LCD_GetDisplayWidth() - 72U, GUI_BACK_BUTTON_Y,
                 GUI_BACK_BUTTON_W, GUI_BACK_BUTTON_H, "BACK",
                 ILI9341_DARKGRAY);
}

static void GUI_DrawFooterButton(uint16_t x, const char *text,
                                 lcd_color_t color) {
  GUI_DrawButton(x, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                 GUI_FOOTER_BUTTON_H, text, color);
}

static void GUI_DrawMainModeLine(void) {
  char text[32];
  char datetime_text[32];
  uint16_t text_x = 0U;
  uint16_t text_width = 0U;

  LCD_FillRect(0U, LAYOUT_MODE_LINE_Y, LCD_GetDisplayWidth(),
               LAYOUT_MODE_LINE_HEIGHT, LAYOUT_COLOR_BG);
  snprintf(text, sizeof(text), "MODE %s",
           GUI_GetAutoModeName(IRRIGATION_CTRL_GetAutoMode()));
  LCD_DrawString(LAYOUT_GRID_MARGIN, LAYOUT_MODE_LINE_Y + 2U, text,
                 LAYOUT_COLOR_TEXT, LAYOUT_COLOR_BG, LAYOUT_FONT_NORMAL);

  GUI_FormatMainDateTime(datetime_text, sizeof(datetime_text));
  snprintf(g_gui_last_datetime_text, sizeof(g_gui_last_datetime_text), "%s",
           datetime_text);
  text_width = (uint16_t)(strlen(datetime_text) * Font_16x8.width);
  if (text_width < LCD_GetDisplayWidth()) {
    text_x = (uint16_t)(LCD_GetDisplayWidth() - text_width - 10U);
  }
  LCD_DrawString(text_x, LAYOUT_MODE_LINE_Y + 2U, datetime_text,
                 LAYOUT_COLOR_ACCENT, LAYOUT_COLOR_BG, LAYOUT_FONT_NORMAL);
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
  char time_text[12];
  uint32_t duration_sec = 0U;
  control_state_t state = IRRIGATION_CTRL_GetState();

  LCD_FillRect(0U, LAYOUT_PARCEL_LINE_Y, LCD_GetDisplayWidth(),
               LAYOUT_PARCEL_LINE_HEIGHT, LAYOUT_COLOR_BG);

  if (state == CTRL_STATE_WAITING) {
    GUI_FormatMMSS(time_text, sizeof(time_text), remaining_sec);
    LCD_DrawString(LAYOUT_GRID_MARGIN, LAYOUT_PARCEL_LINE_Y + 2U,
                   "WAIT BETWEEN PARCELS", LAYOUT_COLOR_WARNING,
                   LAYOUT_COLOR_BG, LAYOUT_FONT_NORMAL);
    snprintf(text, sizeof(text), "LEFT %s", time_text);
    LCD_DrawString(210U, LAYOUT_PARCEL_LINE_Y + 2U, text, LAYOUT_COLOR_TEXT,
                   LAYOUT_COLOR_BG, LAYOUT_FONT_NORMAL);
  } else if (state == CTRL_STATE_ERROR ||
             state == CTRL_STATE_EMERGENCY_STOP) {
    LCD_DrawString(LAYOUT_GRID_MARGIN, LAYOUT_PARCEL_LINE_Y + 2U,
                   IRRIGATION_CTRL_GetActiveAlarmText(), LAYOUT_COLOR_ERROR,
                   LAYOUT_COLOR_BG, LAYOUT_FONT_NORMAL);
  } else if (IRRIGATION_CTRL_IsRunning() != 0U && current_parcel != 0U) {
    duration_sec = GUI_GetMainProgressDurationSec(current_parcel, remaining_sec);
    snprintf(text, sizeof(text), "PARCEL %u", current_parcel);
    LCD_DrawString(LAYOUT_GRID_MARGIN, LAYOUT_PARCEL_LINE_Y + 2U, text,
                   LAYOUT_COLOR_WARNING, LAYOUT_COLOR_BG, LAYOUT_FONT_NORMAL);

    GUI_FormatMMSS(time_text, sizeof(time_text), duration_sec);
    snprintf(text, sizeof(text), "DUR %s", time_text);
    LCD_DrawString(180U, LAYOUT_PARCEL_LINE_Y + 2U, text, LAYOUT_COLOR_TEXT,
                   LAYOUT_COLOR_BG, LAYOUT_FONT_NORMAL);
  } else {
    LCD_DrawString(LAYOUT_GRID_MARGIN, LAYOUT_PARCEL_LINE_Y + 2U,
                   "SYSTEM IDLE", LAYOUT_COLOR_TEXT_DIM, LAYOUT_COLOR_BG,
                   LAYOUT_FONT_NORMAL);
  }
}

static void GUI_DrawMainActions(void) {
  const char *run_text =
      (IRRIGATION_CTRL_IsRunning() != 0U) ? "STOP" : "START";
  lcd_color_t run_color =
      (IRRIGATION_CTRL_IsRunning() != 0U) ? LAYOUT_COLOR_ERROR : LAYOUT_COLOR_SUCCESS;

  /* Three buttons: START/STOP, MANUAL, SETTINGS */
  uint16_t btn_y = LAYOUT_ACTION_BUTTONS_Y;
  uint16_t btn_w = LAYOUT_ACTION_BUTTON_WIDTH;
  uint16_t total_width = (3U * btn_w) + (2U * LAYOUT_ACTION_BUTTON_GAP);
  uint16_t start_x = (LCD_GetDisplayWidth() - total_width) / 2U;

  GUI_DrawButton(start_x, btn_y, btn_w, LAYOUT_ACTION_BUTTON_HEIGHT, run_text,
                 run_color);
  GUI_DrawButton(start_x + btn_w + LAYOUT_ACTION_BUTTON_GAP, btn_y, btn_w,
                 LAYOUT_ACTION_BUTTON_HEIGHT, "MANUAL", LAYOUT_COLOR_BG_LIGHT);
  GUI_DrawButton(start_x + (2U * (btn_w + LAYOUT_ACTION_BUTTON_GAP)), btn_y,
                 btn_w, LAYOUT_ACTION_BUTTON_HEIGHT, "SETTINGS",
                 LAYOUT_COLOR_PRIMARY_DARK);
}

static void GUI_DrawMainProgress(uint8_t parcel_id, uint32_t remaining_sec) {
  char text[32];
  char left_text[12];
  char duration_text[12];
  uint32_t duration_sec = 0U;
  uint32_t elapsed_sec = 0U;
  uint32_t progress_width = 0U;
  uint16_t bar_x = LAYOUT_PROGRESSBAR_X;
  uint16_t bar_y = LAYOUT_PROGRESSBAR_Y;
  uint16_t bar_w = LAYOUT_PROGRESSBAR_WIDTH;
  uint16_t bar_h = LAYOUT_PROGRESSBAR_HEIGHT;
  control_state_t state = IRRIGATION_CTRL_GetState();

  LCD_FillRect(bar_x, bar_y, bar_w, bar_h, LAYOUT_COLOR_BG);
  LCD_DrawRect(bar_x, bar_y, bar_w, bar_h, LAYOUT_COLOR_TEXT_DIM);

  if (state == CTRL_STATE_WAITING) {
    GUI_FormatMMSS(left_text, sizeof(left_text), remaining_sec);
    snprintf(text, sizeof(text), "WAIT %s", left_text);
  } else if (parcel_id != 0U && IRRIGATION_CTRL_IsRunning() != 0U) {
    duration_sec = GUI_GetMainProgressDurationSec(parcel_id, remaining_sec);
    if (duration_sec != 0U) {
      if (remaining_sec > duration_sec) {
        remaining_sec = duration_sec;
      }
      elapsed_sec = duration_sec - remaining_sec;
      progress_width =
          (uint32_t)((bar_w - 2U) * elapsed_sec / duration_sec);
    }

    GUI_FormatMMSS(left_text, sizeof(left_text), remaining_sec);
    GUI_FormatMMSS(duration_text, sizeof(duration_text), duration_sec);
    snprintf(text, sizeof(text), "LEFT %s / %s", left_text, duration_text);
  } else {
    snprintf(text, sizeof(text), "READY");
  }

  if (progress_width > 0U) {
    LCD_FillRect(bar_x + 1U, bar_y + 1U, (uint16_t)progress_width, bar_h - 2U,
                 (state == CTRL_STATE_WAITING) ? LAYOUT_COLOR_WARNING
                                               : LAYOUT_COLOR_PRIMARY);
  }

  /* Center text in progress bar */
  uint16_t text_width = (uint16_t)(strlen(text) * Font_16x8.width);
  uint16_t text_x = bar_x + (bar_w - text_width) / 2U;
  LCD_DrawString(text_x, bar_y + 2U, text, LAYOUT_COLOR_TEXT, LAYOUT_COLOR_BG,
                 LAYOUT_FONT_NORMAL);
}

static void GUI_DrawManualScreen(void) {
  uint8_t page_count =
      GUI_GetPageCount(VALVE_COUNT, GUI_LIST_PAGE_SIZE);
  uint8_t page_start = GUI_GetPageStartIndex(g_gui_state.manual_page,
                                             GUI_LIST_PAGE_SIZE);
  char page_text[20];

  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("MANUAL VALVES");
  GUI_DrawBackButton();

  for (uint8_t slot = 0U; slot < GUI_LIST_PAGE_SIZE; slot++) {
    uint8_t valve_id = page_start + slot;
    if (valve_id > VALVE_COUNT) {
      break;
    }
    GUI_DrawManualValveButton(valve_id);
  }

  GUI_DrawFooterButton(10U, "ALL OFF", ILI9341_DARKRED);
  snprintf(page_text, sizeof(page_text), "PAGE %u/%u",
           (unsigned int)(g_gui_state.manual_page + 1U),
           (unsigned int)page_count);
  GUI_DrawFooterButton(165U, page_text,
                       (page_count > 1U) ? ILI9341_NAVY : ILI9341_DARKGRAY);
}

static void GUI_DrawSettingsScreen(void) {
  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("SETTINGS");
  GUI_DrawBackButton();

  /* 2x5 Grid - 10 buttons */
  GUI_DrawButton(10U, 38U, 145U, 30U, "PH", ILI9341_DARKGRAY);
  GUI_DrawButton(165U, 38U, 145U, 30U, "EC", ILI9341_DARKGRAY);
  GUI_DrawButton(10U, 74U, 145U, 30U, "PARCEL", ILI9341_DARKGRAY);
  GUI_DrawButton(165U, 74U, 145U, 30U, "CALIB", ILI9341_DARKGRAY);
  GUI_DrawButton(10U, 110U, 145U, 30U, "PROGRAM", ILI9341_DARKGRAY);
  GUI_DrawButton(165U, 110U, 145U, 30U, "RTC", ILI9341_DARKGRAY);
  GUI_DrawButton(10U, 146U, 145U, 30U, "AUTO", ILI9341_DARKGRAY);
  GUI_DrawButton(165U, 146U, 145U, 30U, "ALARM", ILI9341_DARKGRAY);
  GUI_DrawButton(10U, 182U, 145U, 30U, "DOSING", ILI9341_DARKGREEN);
  GUI_DrawButton(165U, 182U, 145U, 30U, "PARAMS", ILI9341_NAVY);
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

static void GUI_DrawUIntAdjustRow(uint16_t y, const char *label, uint32_t value,
                                  const char *unit) {
  char value_text[24];

  LCD_DrawString(10U, y + 8U, label, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);
  LCD_DrawRect(110U, y, 90U, 30U, ILI9341_GRAY);

  if (unit != NULL && unit[0] != '\0') {
    snprintf(value_text, sizeof(value_text), "%lu %s", (unsigned long)value, unit);
  } else {
    snprintf(value_text, sizeof(value_text), "%lu", (unsigned long)value);
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

  GUI_DrawFooterButton(10U, "KAYDET", ILI9341_DARKGREEN);
  GUI_DrawFooterButton(165U, "BACK", ILI9341_NAVY);
}

static void GUI_DrawECSettingsScreen(void) {
  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("EC SETTINGS");
  GUI_DrawBackButton();

  GUI_DrawAdjustRow(46U, "TARGET", g_gui_state.ec_draft.target, "MS");
  GUI_DrawAdjustRow(88U, "MIN", g_gui_state.ec_draft.min_limit, "MS");
  GUI_DrawAdjustRow(130U, "MAX", g_gui_state.ec_draft.max_limit, "MS");

  GUI_DrawFooterButton(10U, "KAYDET", ILI9341_DARKGREEN);
  GUI_DrawFooterButton(165U, "BACK", ILI9341_NAVY);
}

static void GUI_DrawParametersScreen(void) {
  ph_control_params_t *ph = &g_gui_state.ph_draft;
  ec_control_params_t *ec = &g_gui_state.ec_draft;
  char page_text[16];

  LCD_Clear(ILI9341_BLACK);
  if (g_gui_state.params_page == 0U) {
    GUI_DrawHeader("PH PARAMS");
  } else {
    GUI_DrawHeader("EC PARAMS");
  }
  GUI_DrawBackButton();

  if (g_gui_state.params_page == 0U) {
    GUI_DrawUIntAdjustRow(46U, "FB DELAY", ph->feedback_delay_ms / 1000U, "S");
    GUI_DrawUIntAdjustRow(88U, "GAIN", ph->response_gain_percent, "%");
    GUI_DrawUIntAdjustRow(130U, "MAX CYC", ph->max_correction_cycles, "");
  } else if (g_gui_state.params_page == 1U) {
    GUI_DrawUIntAdjustRow(46U, "FB DELAY", ec->feedback_delay_ms / 1000U, "S");
    GUI_DrawUIntAdjustRow(88U, "GAIN", ec->response_gain_percent, "%");
    GUI_DrawUIntAdjustRow(130U, "MAX CYC", ec->max_correction_cycles, "");
  }

  GUI_DrawFooterButton(10U, "KAYDET", ILI9341_DARKGREEN);
  snprintf(page_text, sizeof(page_text), "PAGE %u/2",
           (unsigned int)(g_gui_state.params_page + 1U));
  GUI_DrawFooterButton(165U, page_text, ILI9341_NAVY);
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

  GUI_DrawFooterButton(10U, "KAYDET", ILI9341_DARKGREEN);
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
  char page_text[20];
  char action_text[20];
  uint8_t page_count =
      GUI_GetPageCount(IRRIGATION_PROGRAM_COUNT, GUI_LIST_PAGE_SIZE);
  uint8_t page_start = GUI_GetPageStartIndex(g_gui_state.programs_page,
                                             GUI_LIST_PAGE_SIZE);

  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("PROGRAM LIST");
  GUI_DrawBackButton();

  for (uint8_t slot = 0U; slot < GUI_LIST_PAGE_SIZE; slot++) {
    uint8_t program_id = page_start + slot;
    irrigation_program_t program = {0};
    uint16_t x = (slot % 2U == 0U) ? 10U : 165U;
    uint16_t y = 38U + (uint16_t)((slot / 2U) * 38U);

    if (program_id > IRRIGATION_PROGRAM_COUNT) {
      break;
    }

    IRRIGATION_CTRL_GetProgram(program_id, &program);
    snprintf(text, sizeof(text), "P%u %s %04u", program_id,
             (program.enabled != 0U) ? "ON" : "OFF", program.start_hhmm);
    GUI_DrawButton(x, y, 145U, 30U, text,
                   (g_gui_state.selected_program == program_id) ? ILI9341_NAVY
                                                                : ILI9341_DARKGRAY);
  }

  if (IRRIGATION_CTRL_IsRunning() != 0U &&
      IRRIGATION_CTRL_GetActiveProgram() == g_gui_state.selected_program) {
    snprintf(action_text, sizeof(action_text), "STOP P%u",
             g_gui_state.selected_program);
    GUI_DrawFooterButton(10U, action_text, ILI9341_RED);
  } else {
    snprintf(action_text, sizeof(action_text), "RUN P%u",
             g_gui_state.selected_program);
    GUI_DrawFooterButton(10U, action_text, ILI9341_DARKGREEN);
  }

  snprintf(page_text, sizeof(page_text), "PAGE %u/%u",
           (unsigned int)(g_gui_state.programs_page + 1U),
           (unsigned int)page_count);
  GUI_DrawFooterButton(165U, page_text,
                       (page_count > 1U) ? ILI9341_NAVY : ILI9341_DARKGRAY);
}

static void GUI_DrawProgramEditScreen(void) {
  char title[24];
  char value_text[24];
  uint16_t footer_x = 10U;

  LCD_Clear(ILI9341_BLACK);
  snprintf(title, sizeof(title), "PROGRAM %u P%u", g_gui_state.selected_program,
           g_gui_state.program_edit_page + 1U);
  GUI_DrawHeader(title);
  GUI_DrawBackButton();

  if (g_gui_state.program_edit_page == 0U) {
    snprintf(value_text, sizeof(value_text), "%s",
             (g_gui_state.program_draft.enabled != 0U) ? "ON" : "OFF");
    GUI_DrawParcelAdjustRow("ENABLED", value_text, GUI_PROGRAM_EDIT_ROW_Y0);
    snprintf(value_text, sizeof(value_text), "%04u", g_gui_state.program_draft.start_hhmm);
    GUI_DrawParcelAdjustRow("START", value_text,
                            (uint16_t)(GUI_PROGRAM_EDIT_ROW_Y0 + GUI_PROGRAM_EDIT_ROW_STEP));
    snprintf(value_text, sizeof(value_text), "%04u", g_gui_state.program_draft.end_hhmm);
    GUI_DrawParcelAdjustRow("END", value_text,
                            (uint16_t)(GUI_PROGRAM_EDIT_ROW_Y0 + (2U * GUI_PROGRAM_EDIT_ROW_STEP)));
    GUI_FormatValveMask(value_text, sizeof(value_text),
                        g_gui_state.program_draft.valve_mask);
    GUI_DrawParcelAdjustRow("MASK", value_text,
                            (uint16_t)(GUI_PROGRAM_EDIT_ROW_Y0 + (3U * GUI_PROGRAM_EDIT_ROW_STEP)));
    snprintf(value_text, sizeof(value_text), "%u MIN",
             g_gui_state.program_draft.irrigation_min);
    GUI_DrawParcelAdjustRow("DUR", value_text,
                            (uint16_t)(GUI_PROGRAM_EDIT_ROW_Y0 + (4U * GUI_PROGRAM_EDIT_ROW_STEP)));
  } else if (g_gui_state.program_edit_page == 1U) {
    snprintf(value_text, sizeof(value_text), "%u MIN",
             g_gui_state.program_draft.wait_min);
    GUI_DrawParcelAdjustRow("WAIT", value_text, GUI_PROGRAM_EDIT_ROW_Y0);
    snprintf(value_text, sizeof(value_text), "%u",
             g_gui_state.program_draft.repeat_count);
    GUI_DrawParcelAdjustRow("REPEAT", value_text,
                            (uint16_t)(GUI_PROGRAM_EDIT_ROW_Y0 + GUI_PROGRAM_EDIT_ROW_STEP));
    GUI_FormatDaysMask(value_text, sizeof(value_text),
                       g_gui_state.program_draft.days_mask);
    GUI_DrawParcelAdjustRow("DAYS", value_text,
                            (uint16_t)(GUI_PROGRAM_EDIT_ROW_Y0 + (2U * GUI_PROGRAM_EDIT_ROW_STEP)));
    snprintf(value_text, sizeof(value_text), "%ld.%02ld",
             (long)(g_gui_state.program_draft.ph_set_x100 / 100),
             (long)labs(g_gui_state.program_draft.ph_set_x100 % 100));
    GUI_DrawParcelAdjustRow("PH", value_text,
                            (uint16_t)(GUI_PROGRAM_EDIT_ROW_Y0 + (3U * GUI_PROGRAM_EDIT_ROW_STEP)));
    snprintf(value_text, sizeof(value_text), "%ld.%02ld",
             (long)(g_gui_state.program_draft.ec_set_x100 / 100),
             (long)labs(g_gui_state.program_draft.ec_set_x100 % 100));
    GUI_DrawParcelAdjustRow("EC", value_text,
                            (uint16_t)(GUI_PROGRAM_EDIT_ROW_Y0 + (4U * GUI_PROGRAM_EDIT_ROW_STEP)));
  } else if (g_gui_state.program_edit_page == 2U) {
    snprintf(value_text, sizeof(value_text), "%u%%",
             g_gui_state.program_draft.fert_ratio_percent[0]);
    GUI_DrawParcelAdjustRow("FERT A", value_text, GUI_PROGRAM_EDIT_ROW_Y0);
    snprintf(value_text, sizeof(value_text), "%u%%",
             g_gui_state.program_draft.fert_ratio_percent[1]);
    GUI_DrawParcelAdjustRow("FERT B", value_text,
                            (uint16_t)(GUI_PROGRAM_EDIT_ROW_Y0 + GUI_PROGRAM_EDIT_ROW_STEP));
    snprintf(value_text, sizeof(value_text), "%u%%",
             g_gui_state.program_draft.fert_ratio_percent[2]);
    GUI_DrawParcelAdjustRow("FERT C", value_text,
                            (uint16_t)(GUI_PROGRAM_EDIT_ROW_Y0 + (2U * GUI_PROGRAM_EDIT_ROW_STEP)));
    snprintf(value_text, sizeof(value_text), "%u%%",
             g_gui_state.program_draft.fert_ratio_percent[3]);
    GUI_DrawParcelAdjustRow("FERT D", value_text,
                            (uint16_t)(GUI_PROGRAM_EDIT_ROW_Y0 + (3U * GUI_PROGRAM_EDIT_ROW_STEP)));
  } else if (g_gui_state.program_edit_page == 3U) {
    snprintf(value_text, sizeof(value_text), "%u SEC",
             g_gui_state.program_draft.pre_flush_sec);
    GUI_DrawParcelAdjustRow("PRE FLUSH", value_text, GUI_PROGRAM_EDIT_ROW_Y0);
    snprintf(value_text, sizeof(value_text), "%u SEC",
             g_gui_state.program_draft.post_flush_sec);
    GUI_DrawParcelAdjustRow("POST FLUSH", value_text,
                            (uint16_t)(GUI_PROGRAM_EDIT_ROW_Y0 + GUI_PROGRAM_EDIT_ROW_STEP));
  }

  GUI_DrawButton(footer_x, GUI_PROGRAM_EDIT_FOOTER_Y, GUI_PROGRAM_EDIT_BUTTON_W,
                 GUI_PROGRAM_EDIT_BUTTON_H, "KAYDET", ILI9341_DARKGREEN);
  footer_x = (uint16_t)(footer_x + GUI_PROGRAM_EDIT_BUTTON_W +
                        GUI_PROGRAM_EDIT_BUTTON_GAP);
  GUI_DrawButton(footer_x, GUI_PROGRAM_EDIT_FOOTER_Y, GUI_PROGRAM_EDIT_BUTTON_W,
                 GUI_PROGRAM_EDIT_BUTTON_H, "PAGE", ILI9341_DARKGRAY);
  footer_x = (uint16_t)(footer_x + GUI_PROGRAM_EDIT_BUTTON_W +
                        GUI_PROGRAM_EDIT_BUTTON_GAP);
  GUI_DrawButton(footer_x, GUI_PROGRAM_EDIT_FOOTER_Y, GUI_PROGRAM_EDIT_BUTTON_W,
                 GUI_PROGRAM_EDIT_BUTTON_H, "LIST", ILI9341_NAVY);
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

  GUI_DrawFooterButton(10U, "KAYDET", ILI9341_DARKGREEN);
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
  char text[64];
  irrigation_runtime_backup_t backup = {0};
  uint8_t ack_required = 0U;
  uint8_t reset_ready = 0U;
  const char *detail_text = IRRIGATION_CTRL_GetAlarmDetailText();
  const char *action_text = IRRIGATION_CTRL_GetAlarmActionText();
  const char *alarm_title = gSystemStatus.alarm_text;

  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("ALARMS");
  GUI_DrawBackButton();

  ack_required = (IRRIGATION_CTRL_HasErrors() != 0U &&
                  IRRIGATION_CTRL_IsAlarmAcknowledged() == 0U)
                     ? 1U
                     : 0U;
  reset_ready = IRRIGATION_CTRL_CanResetAlarm();

  if (IRRIGATION_CTRL_HasErrors() == 0U) {
    if (gSystemStatus.rtc_ok == 0U) {
      alarm_title = "RTC AYARI";
      detail_text = "RTC ayari yok. Zamanli program baslatilamiyor.";
      action_text = "Saat ve tarihi ayarlayin, sonra tekrar deneyin.";
    } else if (gSystemStatus.ph_sensor_ok == 0U) {
      alarm_title = "PH SENSOR";
      detail_text = "pH verisi gecersiz. Prob ve baglantiyi kontrol edin.";
      action_text = "Sensor toparlaninca alarm ekrani kendini yeniler.";
    } else if (gSystemStatus.ec_sensor_ok == 0U) {
      alarm_title = "EC SENSOR";
      detail_text = "EC verisi gecersiz. Prob ve baglantiyi kontrol edin.";
      action_text = "Sensor toparlaninca alarm ekrani kendini yeniler.";
    } else if (gSystemStatus.eeprom_crc_ok == 0U) {
      alarm_title = "EEPROM CRC";
      detail_text = "Kayitli ayar verisi dogrulanamadi.";
      action_text = "Varsayilan ayarlari kontrol edip tekrar kaydedin.";
    }
  }

  snprintf(text, sizeof(text), "ALARM %s", alarm_title);
  LCD_DrawString(10U, 42U, text, ILI9341_YELLOW, ILI9341_BLACK, &Font_16x8);
  GUI_DrawTextWrapped2(10U, 62U, 300U, detail_text, ILI9341_WHITE,
                       ILI9341_BLACK, &Font_8x5);
  GUI_DrawTextWrapped2(10U, 86U, 300U, action_text, ILI9341_CYAN,
                       ILI9341_BLACK, &Font_8x5);
  snprintf(text, sizeof(text), "ERR %u", gSystemStatus.error_code);
  LCD_DrawString(10U, 112U, text, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);
  snprintf(text, sizeof(text), "PH %s  EC %s",
           (gSystemStatus.ph_sensor_ok != 0U) ? "OK" : "ERR",
           (gSystemStatus.ec_sensor_ok != 0U) ? "OK" : "ERR");
  LCD_DrawString(10U, 136U, text, ILI9341_CYAN, ILI9341_BLACK, &Font_16x8);
  snprintf(text, sizeof(text), "RTC %s  EEPROM %s",
           (gSystemStatus.rtc_ok != 0U) ? "OK" : "ERR",
           (gSystemStatus.eeprom_crc_ok != 0U) ? "OK" : "CRC");
  LCD_DrawString(10U, 158U, text, ILI9341_GREEN, ILI9341_BLACK, &Font_16x8);

  IRRIGATION_CTRL_GetRuntimeBackup(&backup);
  snprintf(text, sizeof(text), "PRG %u VALVE %u LEFT %u",
           backup.active_program_id, backup.active_valve_id, backup.remaining_sec);
  LCD_DrawString(10U, 180U, text, ILI9341_WHITE, ILI9341_BLACK, &Font_8x5);

  LCD_FillRect(10U, 194U, 300U, 12U, ILI9341_BLACK);
  LCD_DrawString(10U, 194U, g_gui_state.notice, ILI9341_YELLOW, ILI9341_BLACK,
                 &Font_8x5);

  GUI_DrawFooterButton(10U, "ONAYLA",
                       (ack_required != 0U) ? ILI9341_ORANGE
                                            : ILI9341_DARKGRAY);
  GUI_DrawFooterButton(165U, "RESET",
                       (reset_ready != 0U) ? ILI9341_DARKGREEN
                                           : ILI9341_DARKGRAY);
}

static void GUI_SetNotice(const char *text) {
  snprintf(g_gui_state.notice, sizeof(g_gui_state.notice), "%s", text);
}

static uint8_t GUI_GetPageCount(uint8_t item_count, uint8_t page_size) {
  if (page_size == 0U) {
    return 1U;
  }
  return (uint8_t)((item_count + page_size - 1U) / page_size);
}

static uint8_t GUI_GetPageStartIndex(uint8_t page, uint8_t page_size) {
  return (uint8_t)(1U + (page * page_size));
}

static void GUI_DrawManualValveButton(uint8_t valve_id) {
  char label[20];
  uint16_t x;
  uint16_t y;
  uint8_t is_open;
  uint8_t local_index;
  uint8_t page_start = GUI_GetPageStartIndex(g_gui_state.manual_page,
                                             GUI_LIST_PAGE_SIZE);

  if (valve_id == 0U || valve_id > VALVE_COUNT) return;
  if (valve_id < page_start ||
      valve_id >= (uint8_t)(page_start + GUI_LIST_PAGE_SIZE)) {
    return;
  }

  local_index = (uint8_t)(valve_id - page_start);
  x = (local_index % 2U == 0U) ? 10U : 165U;
  y = 42U + (uint16_t)((local_index / 2U) * 39U);
  is_open = (VALVES_GetState(valve_id) == VALVE_STATE_OPEN);

  snprintf(label, sizeof(label), "%s %s", GUI_GetValveLabel(valve_id),
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

static uint32_t GUI_GetMainProgressDurationSec(uint8_t parcel_id,
                                               uint32_t remaining_sec) {
  uint32_t elapsed_sec = IRRIGATION_CTRL_GetCurrentRunTime();
  uint32_t duration_sec = 0U;

  if (IRRIGATION_CTRL_IsRunning() == 0U) {
    return 0U;
  }

  if (elapsed_sec != 0U || remaining_sec != 0U) {
    duration_sec = elapsed_sec + remaining_sec;
  }

  if (duration_sec == 0U && parcel_id != 0U) {
    duration_sec = PARCELS_GetDuration(parcel_id);
  }

  return duration_sec;
}

static const char *GUI_GetValveLabel(uint8_t valve_id) {
  switch (valve_id) {
  case DOSING_VALVE_ACID_ID:
    return "ACID";
  case DOSING_VALVE_FERT_A_ID:
    return "FERT A";
  case DOSING_VALVE_FERT_B_ID:
    return "FERT B";
  case DOSING_VALVE_FERT_C_ID:
    return "FERT C";
  case DOSING_VALVE_FERT_D_ID:
    return "FERT D";
  default:
    break;
  }

  if (valve_id >= 1U && valve_id <= PARCEL_VALVE_COUNT) {
    static char parcel_label[8];
    snprintf(parcel_label, sizeof(parcel_label), "P%u", valve_id);
    return parcel_label;
  }

  return "V?";
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
  /* Button coordinates match GUI_DrawMainActions */
  uint16_t btn_y = LAYOUT_ACTION_BUTTONS_Y;
  uint16_t btn_w = LAYOUT_ACTION_BUTTON_WIDTH;
  uint16_t btn_h = LAYOUT_ACTION_BUTTON_HEIGHT;
  uint16_t btn_gap = LAYOUT_ACTION_BUTTON_GAP;
  uint16_t total_width = (3U * btn_w) + (2U * btn_gap);
  uint16_t start_x = (LCD_GetDisplayWidth() - total_width) / 2U;

  /* START/STOP button */
  if (GUI_PointInRect(point, start_x, btn_y, btn_w, btn_h) != 0U) {
    if (IRRIGATION_CTRL_IsRunning() != 0U) {
      IRRIGATION_CTRL_Stop();
    } else {
      IRRIGATION_CTRL_Start();
    }
    return;
  }

  /* MANUAL button */
  if (GUI_PointInRect(point, start_x + btn_w + btn_gap, btn_y, btn_w, btn_h) != 0U) {
    GUI_NavigateTo(SCREEN_MANUAL);
    return;
  }

  /* SETTINGS button */
  if (GUI_PointInRect(point, start_x + (2U * (btn_w + btn_gap)), btn_y, btn_w, btn_h) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
  }
}

static void GUI_HandleManualTouch(const touch_point_t *point) {
  uint8_t page_count =
      GUI_GetPageCount(VALVE_COUNT, GUI_LIST_PAGE_SIZE);
  uint8_t page_start = GUI_GetPageStartIndex(g_gui_state.manual_page,
                                             GUI_LIST_PAGE_SIZE);

  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, GUI_BACK_BUTTON_Y,
                      GUI_BACK_BUTTON_W, GUI_BACK_BUTTON_H) != 0U) {
    GUI_NavigateTo(SCREEN_MAIN);
    return;
  }

  if (GUI_PointInRect(point, 10U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
    IRRIGATION_CTRL_Stop();
    VALVES_CloseAll();
    return;
  }

  if (GUI_PointInRect(point, 165U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
    if (page_count > 1U) {
      g_gui_state.manual_page =
          (uint8_t)((g_gui_state.manual_page + 1U) % page_count);
      GUI_DrawManualScreen();
    }
    return;
  }

  for (uint8_t slot = 0U; slot < GUI_LIST_PAGE_SIZE; slot++) {
    uint8_t valve_id = page_start + slot;
    uint16_t x = (slot % 2U == 0U) ? 10U : 165U;
    uint16_t y = 42U + (uint16_t)((slot / 2U) * 39U);

    if (valve_id > VALVE_COUNT) {
      break;
    }

    if (GUI_PointInRect(point, x, y, 145U, 32U) == 0U) continue;

    if (IRRIGATION_CTRL_IsRunning() != 0U) {
      IRRIGATION_CTRL_Stop();
    }

    if (VALVES_GetState(valve_id) == VALVE_STATE_OPEN) {
      VALVES_ManualClose(valve_id);
    } else {
      VALVES_ManualOpen(valve_id);
    }
    GUI_DrawManualValveButton(valve_id);
    return;
  }
}

static void GUI_HandleSettingsTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, GUI_BACK_BUTTON_Y,
                      GUI_BACK_BUTTON_W, GUI_BACK_BUTTON_H) != 0U) {
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

  if (GUI_PointInRect(point, 10U, 182U, 145U, 30U) != 0U) {
    GUI_NavigateTo(SCREEN_DOSING);
    return;
  }

  if (GUI_PointInRect(point, 165U, 182U, 145U, 30U) != 0U) {
    GUI_NavigateTo(SCREEN_PARAMETERS);
    return;
  }
}

static void GUI_HandlePHSettingsTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, GUI_BACK_BUTTON_Y,
                      GUI_BACK_BUTTON_W, GUI_BACK_BUTTON_H) != 0U ||
      GUI_PointInRect(point, 165U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
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
  } else if (GUI_PointInRect(point, 10U, GUI_FOOTER_BUTTON_Y,
                             GUI_FOOTER_BUTTON_W, GUI_FOOTER_BUTTON_H) != 0U) {
    IRRIGATION_CTRL_SetPHParams(g_gui_state.ph_draft.target,
                                g_gui_state.ph_draft.min_limit,
                                g_gui_state.ph_draft.max_limit,
                                g_gui_state.ph_draft.hysteresis);
    IRRIGATION_CTRL_MaintenanceTask();
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  GUI_DrawPHSettingsScreen();
}

static void GUI_HandleECSettingsTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, GUI_BACK_BUTTON_Y,
                      GUI_BACK_BUTTON_W, GUI_BACK_BUTTON_H) != 0U ||
      GUI_PointInRect(point, 165U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
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
  } else if (GUI_PointInRect(point, 10U, GUI_FOOTER_BUTTON_Y,
                             GUI_FOOTER_BUTTON_W, GUI_FOOTER_BUTTON_H) != 0U) {
    IRRIGATION_CTRL_SetECParams(g_gui_state.ec_draft.target,
                                g_gui_state.ec_draft.min_limit,
                                g_gui_state.ec_draft.max_limit,
                                g_gui_state.ec_draft.hysteresis);
    IRRIGATION_CTRL_MaintenanceTask();
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  GUI_DrawECSettingsScreen();
}

static void GUI_HandleParametersTouch(const touch_point_t *point) {
  ph_control_params_t *ph = &g_gui_state.ph_draft;
  ec_control_params_t *ec = &g_gui_state.ec_draft;
  uint8_t minus_pressed = 0U;
  uint8_t plus_pressed = 0U;

  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, GUI_BACK_BUTTON_Y,
                      GUI_BACK_BUTTON_W, GUI_BACK_BUTTON_H) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 10U, GUI_FOOTER_BUTTON_Y,
                      GUI_FOOTER_BUTTON_W, GUI_FOOTER_BUTTON_H) != 0U) {
    IRRIGATION_CTRL_SetPHDosingResponse(ph->feedback_delay_ms,
                                        ph->response_gain_percent,
                                        ph->max_correction_cycles);
    IRRIGATION_CTRL_SetECDosingResponse(ec->feedback_delay_ms,
                                        ec->response_gain_percent,
                                        ec->max_correction_cycles);
    IRRIGATION_CTRL_SaveDosingResponseParams();
    IRRIGATION_CTRL_MaintenanceTask();
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 165U, GUI_FOOTER_BUTTON_Y,
                      GUI_FOOTER_BUTTON_W, GUI_FOOTER_BUTTON_H) != 0U) {
    g_gui_state.params_page = (uint8_t)((g_gui_state.params_page + 1U) % 2U);
    GUI_DrawParametersScreen();
    return;
  }

  minus_pressed = GUI_PointInRect(point, 215U, 46U, 40U, 30U);
  plus_pressed = GUI_PointInRect(point, 265U, 46U, 40U, 30U);
  if (minus_pressed != 0U || plus_pressed != 0U) {
    uint32_t *delay_ms = (g_gui_state.params_page == 0U)
                             ? &ph->feedback_delay_ms
                             : &ec->feedback_delay_ms;
    uint32_t delay_sec = *delay_ms / 1000U;

    if (minus_pressed != 0U && delay_sec >= 5U) {
      delay_sec -= 5U;
    } else if (plus_pressed != 0U && delay_sec <= 595U) {
      delay_sec += 5U;
    }
    *delay_ms = delay_sec * 1000U;
    GUI_DrawParametersScreen();
    return;
  }

  minus_pressed = GUI_PointInRect(point, 215U, 88U, 40U, 30U);
  plus_pressed = GUI_PointInRect(point, 265U, 88U, 40U, 30U);
  if (minus_pressed != 0U || plus_pressed != 0U) {
    uint8_t *gain = (g_gui_state.params_page == 0U)
                        ? &ph->response_gain_percent
                        : &ec->response_gain_percent;

    if (minus_pressed != 0U && *gain > 10U) {
      *gain = (uint8_t)(*gain - 5U);
    } else if (plus_pressed != 0U && *gain < 100U) {
      *gain = (uint8_t)(*gain + 5U);
    }
    GUI_DrawParametersScreen();
    return;
  }

  minus_pressed = GUI_PointInRect(point, 215U, 130U, 40U, 30U);
  plus_pressed = GUI_PointInRect(point, 265U, 130U, 40U, 30U);
  if (minus_pressed != 0U || plus_pressed != 0U) {
    uint8_t *max_cycles = (g_gui_state.params_page == 0U)
                              ? &ph->max_correction_cycles
                              : &ec->max_correction_cycles;

    if (minus_pressed != 0U && *max_cycles > 0U) {
      (*max_cycles)--;
    } else if (plus_pressed != 0U && *max_cycles < 10U) {
      (*max_cycles)++;
    }
    GUI_DrawParametersScreen();
    return;
  }

  GUI_DrawParametersScreen();
}

static void GUI_HandleParcelSettingsTouch(const touch_point_t *point) {
  uint8_t reload_selected_parcel = 0U;

  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, GUI_BACK_BUTTON_Y,
                      GUI_BACK_BUTTON_W, GUI_BACK_BUTTON_H) != 0U ||
      GUI_PointInRect(point, 165U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
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
  } else if (GUI_PointInRect(point, 10U, GUI_FOOTER_BUTTON_Y,
                             GUI_FOOTER_BUTTON_W, GUI_FOOTER_BUTTON_H) != 0U) {
    IRRIGATION_CTRL_SetParcelConfig(g_gui_state.selected_parcel,
                                    g_gui_state.parcel_duration_sec,
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

  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, GUI_BACK_BUTTON_Y,
                      GUI_BACK_BUTTON_W, GUI_BACK_BUTTON_H) != 0U) {
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
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, GUI_BACK_BUTTON_Y,
                      GUI_BACK_BUTTON_W, GUI_BACK_BUTTON_H) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 10U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 165U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
    GUI_NavigateTo(SCREEN_MAIN);
  }
}

static void GUI_HandleProgramsTouch(const touch_point_t *point) {
  uint8_t page_count =
      GUI_GetPageCount(IRRIGATION_PROGRAM_COUNT, GUI_LIST_PAGE_SIZE);
  uint8_t page_start = GUI_GetPageStartIndex(g_gui_state.programs_page,
                                             GUI_LIST_PAGE_SIZE);

  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, GUI_BACK_BUTTON_Y,
                      GUI_BACK_BUTTON_W, GUI_BACK_BUTTON_H) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 10U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
    if (IRRIGATION_CTRL_IsRunning() != 0U &&
        IRRIGATION_CTRL_GetActiveProgram() == g_gui_state.selected_program) {
      IRRIGATION_CTRL_Stop();
    } else {
      IRRIGATION_CTRL_StartProgram(g_gui_state.selected_program);
    }
    GUI_DrawProgramsScreen();
    return;
  }

  if (GUI_PointInRect(point, 165U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
    if (page_count > 1U) {
      g_gui_state.programs_page =
          (uint8_t)((g_gui_state.programs_page + 1U) % page_count);
      GUI_DrawProgramsScreen();
    }
    return;
  }

  for (uint8_t slot = 0U; slot < GUI_LIST_PAGE_SIZE; slot++) {
    uint8_t program_id = page_start + slot;
    uint16_t x = (slot % 2U == 0U) ? 10U : 165U;
    uint16_t y = 38U + (uint16_t)((slot / 2U) * 38U);

    if (program_id > IRRIGATION_PROGRAM_COUNT) {
      break;
    }

    if (GUI_PointInRect(point, x, y, 145U, 30U) == 0U) continue;

    if (g_gui_state.selected_program == program_id) {
      GUI_NavigateTo(SCREEN_PROGRAM_EDIT);
      return;
    }

    g_gui_state.selected_program = program_id;
    GUI_DrawProgramsScreen();
    return;
  }
}

static void GUI_HandleProgramEditTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, GUI_BACK_BUTTON_Y,
                      GUI_BACK_BUTTON_W, GUI_BACK_BUTTON_H) != 0U ||
      GUI_PointInRect(point,
                      (uint16_t)(10U + (2U * (GUI_PROGRAM_EDIT_BUTTON_W +
                                             GUI_PROGRAM_EDIT_BUTTON_GAP))),
                      GUI_PROGRAM_EDIT_FOOTER_Y, GUI_PROGRAM_EDIT_BUTTON_W,
                      GUI_PROGRAM_EDIT_BUTTON_H) != 0U) {
    GUI_NavigateTo(SCREEN_PROGRAMS);
    return;
  }

  if (GUI_PointInRect(point, (uint16_t)(10U + GUI_PROGRAM_EDIT_BUTTON_W +
                                        GUI_PROGRAM_EDIT_BUTTON_GAP),
                      GUI_PROGRAM_EDIT_FOOTER_Y, GUI_PROGRAM_EDIT_BUTTON_W,
                      GUI_PROGRAM_EDIT_BUTTON_H) != 0U) {
    g_gui_state.program_edit_page =
        (uint8_t)((g_gui_state.program_edit_page + 1U) %
                  GUI_PROGRAM_EDIT_PAGE_COUNT);
    GUI_DrawProgramEditScreen();
    return;
  }

  if (GUI_PointInRect(point, 10U, GUI_PROGRAM_EDIT_FOOTER_Y,
                      GUI_PROGRAM_EDIT_BUTTON_W,
                      GUI_PROGRAM_EDIT_BUTTON_H) != 0U) {
    IRRIGATION_CTRL_SetProgram(g_gui_state.selected_program,
                               &g_gui_state.program_draft);
    IRRIGATION_CTRL_MaintenanceTask();
    GUI_NavigateTo(SCREEN_PROGRAMS);
    return;
  }

  for (uint8_t row = 0U; row < GUI_PROGRAM_EDIT_ROW_COUNT; row++) {
    uint16_t y =
        (uint16_t)(GUI_PROGRAM_EDIT_ROW_Y0 + (row * GUI_PROGRAM_EDIT_ROW_STEP));
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
    } else if (g_gui_state.program_edit_page == 1U) {
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
    } else if (g_gui_state.program_edit_page == 2U &&
               row < IRRIGATION_EC_CHANNEL_COUNT) {
      uint8_t *ratio = &g_gui_state.program_draft.fert_ratio_percent[row];

      if (minus_pressed != 0U && *ratio >= 5U) {
        *ratio = (uint8_t)(*ratio - 5U);
      } else if (plus_pressed != 0U && *ratio <= 95U) {
        *ratio = (uint8_t)(*ratio + 5U);
      }
    } else if (g_gui_state.program_edit_page == 3U) {
      uint16_t *flush_sec = NULL;

      if (row == 0U) {
        flush_sec = &g_gui_state.program_draft.pre_flush_sec;
      } else if (row == 1U) {
        flush_sec = &g_gui_state.program_draft.post_flush_sec;
      }

      if (flush_sec != NULL) {
        if (minus_pressed != 0U && *flush_sec >= 15U) {
          *flush_sec = (uint16_t)(*flush_sec - 15U);
        } else if (plus_pressed != 0U &&
                   *flush_sec <= (IRRIGATION_MAX_FLUSH_SEC - 15U)) {
          *flush_sec = (uint16_t)(*flush_sec + 15U);
        }
      }
    }

    GUI_DrawProgramEditScreen();
    return;
  }
}

static void GUI_HandleRTCSettingsTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, GUI_BACK_BUTTON_Y,
                      GUI_BACK_BUTTON_W, GUI_BACK_BUTTON_H) != 0U ||
      GUI_PointInRect(point, 165U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 10U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
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
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, GUI_BACK_BUTTON_Y,
                      GUI_BACK_BUTTON_W, GUI_BACK_BUTTON_H) != 0U ||
      GUI_PointInRect(point, 10U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 165U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
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
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, GUI_BACK_BUTTON_Y,
                      GUI_BACK_BUTTON_W, GUI_BACK_BUTTON_H) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 10U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
    (void)IRRIGATION_CTRL_AcknowledgeAlarm();
    GUI_SetNotice(IRRIGATION_CTRL_GetAlarmActionText());
    BUZZER_BeepShort();
    GUI_DrawAlarmsScreen();
    return;
  }

  if (GUI_PointInRect(point, 165U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
    if (IRRIGATION_CTRL_ResetAlarm() != 0U) {
      GUI_SetNotice("ALARM TEMIZLENDI");
      BUZZER_BeepSuccess();
    } else if (IRRIGATION_CTRL_CanResetAlarm() == 0U &&
               IRRIGATION_CTRL_HasErrors() != 0U) {
      GUI_SetNotice("ONCE ONAYLA");
      BUZZER_BeepWarning();
    } else {
      GUI_SetNotice(IRRIGATION_CTRL_GetActiveAlarmText());
      BUZZER_BeepWarning();
    }
    GUI_DrawAlarmsScreen();
  }
}

/**
 * @brief  Handle touch events for Dosing Screen
 */
static void GUI_HandleDosingTouch(const touch_point_t *point) {
  uint16_t btn_width = 145U;
  uint16_t btn_height = 40U;
  uint16_t btn_gap = 8U;
  uint16_t start_x = 10U;
  uint16_t start_y = 45U;
  uint16_t screen_w = LCD_GetDisplayWidth();

  /* Back button - top right */
  if (GUI_PointInRect(point, screen_w - 72U, GUI_BACK_BUTTON_Y,
                      GUI_BACK_BUTTON_W, GUI_BACK_BUTTON_H) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  /* Footer: MAIN button - bottom left */
  if (GUI_PointInRect(point, 10U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
    GUI_NavigateTo(SCREEN_MAIN);
    return;
  }

  /* Footer: SETTINGS button - bottom right */
  if (GUI_PointInRect(point, 165U, GUI_FOOTER_BUTTON_Y, GUI_FOOTER_BUTTON_W,
                      GUI_FOOTER_BUTTON_H) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  /* ACID Valve - Top left: (10,45) to (155,85) */
  if (GUI_PointInRect(point, start_x, start_y, btn_width, btn_height) != 0U) {
    GUI_ToggleDosingGPIO(DOSING_VALVE_ACID_ID);
    HAL_Delay(50);
    GUI_DrawDosingScreen();
    return;
  }

  /* FERT A Valve - Top right: (163,45) to (308,85) */
  if (GUI_PointInRect(point, start_x + btn_width + btn_gap, start_y, btn_width, btn_height) != 0U) {
    GUI_ToggleDosingGPIO(DOSING_VALVE_FERT_A_ID);
    HAL_Delay(50);
    GUI_DrawDosingScreen();
    return;
  }

  /* FERT B Valve - Middle left: (10,93) to (155,133) */
  if (GUI_PointInRect(point, start_x, start_y + btn_height + btn_gap, btn_width, btn_height) != 0U) {
    GUI_ToggleDosingGPIO(DOSING_VALVE_FERT_B_ID);
    HAL_Delay(50);
    GUI_DrawDosingScreen();
    return;
  }

  /* FERT C Valve - Middle right: (163,93) to (308,133) */
  if (GUI_PointInRect(point, start_x + btn_width + btn_gap, start_y + btn_height + btn_gap, btn_width, btn_height) != 0U) {
    GUI_ToggleDosingGPIO(DOSING_VALVE_FERT_C_ID);
    HAL_Delay(50);
    GUI_DrawDosingScreen();
    return;
  }

  /* FERT D Valve - Bottom left: (10,141) to (155,181) */
  if (GUI_PointInRect(point, start_x, start_y + 2 * (btn_height + btn_gap), btn_width, btn_height) != 0U) {
    GUI_ToggleDosingGPIO(DOSING_VALVE_FERT_D_ID);
    HAL_Delay(50);
    GUI_DrawDosingScreen();
    return;
  }
}

/**
 * @brief  Draw Dosing Valve Control Screen
 */
static void GUI_DrawDosingScreen(void) {
  char label[32];
  uint16_t btn_width = 145U;
  uint16_t btn_height = 40U;
  uint16_t btn_gap = 8U;
  uint16_t start_x = 10U;
  uint16_t start_y = 45U;
  dosing_channel_status_t dosing_status = {0};

  LCD_Clear(LAYOUT_COLOR_BG);
  GUI_DrawHeader("DOSING VALVES");
  GUI_DrawBackButton();

  VALVES_GetDosingStatus(DOSING_VALVE_ACID_ID, &dosing_status);
  GPIO_PinState acid_gpio = GUI_ReadDosingGPIO(DOSING_VALVE_ACID_ID);
  lcd_color_t acid_color = (acid_gpio == GPIO_PIN_SET) ?
                           LAYOUT_COLOR_SUCCESS : ILI9341_DARKGRAY;

  GUI_DrawButton(start_x, start_y, btn_width, btn_height, "ACID", acid_color);
  snprintf(label, sizeof(label), "%02uHZ %3u%%",
           dosing_status.frequency_hz, dosing_status.applied_duty_percent);
  LCD_DrawString(start_x + 6U, start_y + 24U, label, LAYOUT_COLOR_TEXT, acid_color, LAYOUT_FONT_SMALL);

  VALVES_GetDosingStatus(DOSING_VALVE_FERT_A_ID, &dosing_status);
  GPIO_PinState fert_a_gpio = GUI_ReadDosingGPIO(DOSING_VALVE_FERT_A_ID);
  lcd_color_t fert_a_color = (fert_a_gpio == GPIO_PIN_SET) ?
                             LAYOUT_COLOR_SUCCESS : ILI9341_DARKGRAY;

  GUI_DrawButton(start_x + btn_width + btn_gap, start_y, btn_width, btn_height, "FERT A", fert_a_color);
  snprintf(label, sizeof(label), "%02uHZ %3u%%",
           dosing_status.frequency_hz, dosing_status.applied_duty_percent);
  LCD_DrawString(start_x + btn_width + btn_gap + 6U, start_y + 24U, label, LAYOUT_COLOR_TEXT, fert_a_color, LAYOUT_FONT_SMALL);

  VALVES_GetDosingStatus(DOSING_VALVE_FERT_B_ID, &dosing_status);
  GPIO_PinState fert_b_gpio = GUI_ReadDosingGPIO(DOSING_VALVE_FERT_B_ID);
  lcd_color_t fert_b_color = (fert_b_gpio == GPIO_PIN_SET) ?
                             LAYOUT_COLOR_SUCCESS : ILI9341_DARKGRAY;

  GUI_DrawButton(start_x, start_y + btn_height + btn_gap, btn_width, btn_height, "FERT B", fert_b_color);
  snprintf(label, sizeof(label), "%02uHZ %3u%%",
           dosing_status.frequency_hz, dosing_status.applied_duty_percent);
  LCD_DrawString(start_x + 6U, start_y + btn_height + btn_gap + 24U, label, LAYOUT_COLOR_TEXT, fert_b_color, LAYOUT_FONT_SMALL);

  VALVES_GetDosingStatus(DOSING_VALVE_FERT_C_ID, &dosing_status);
  GPIO_PinState fert_c_gpio = GUI_ReadDosingGPIO(DOSING_VALVE_FERT_C_ID);
  lcd_color_t fert_c_color = (fert_c_gpio == GPIO_PIN_SET) ?
                             LAYOUT_COLOR_SUCCESS : ILI9341_DARKGRAY;

  GUI_DrawButton(start_x + btn_width + btn_gap, start_y + btn_height + btn_gap, btn_width, btn_height, "FERT C", fert_c_color);
  snprintf(label, sizeof(label), "%02uHZ %3u%%",
           dosing_status.frequency_hz, dosing_status.applied_duty_percent);
  LCD_DrawString(start_x + btn_width + btn_gap + 6U, start_y + btn_height + btn_gap + 24U, label, LAYOUT_COLOR_TEXT, fert_c_color, LAYOUT_FONT_SMALL);

  VALVES_GetDosingStatus(DOSING_VALVE_FERT_D_ID, &dosing_status);
  GPIO_PinState fert_d_gpio = GUI_ReadDosingGPIO(DOSING_VALVE_FERT_D_ID);
  lcd_color_t fert_d_color = (fert_d_gpio == GPIO_PIN_SET) ?
                             LAYOUT_COLOR_SUCCESS : ILI9341_DARKGRAY;

  GUI_DrawButton(start_x, start_y + 2 * (btn_height + btn_gap), btn_width, btn_height, "FERT D", fert_d_color);
  snprintf(label, sizeof(label), "%02uHZ %3u%%",
           dosing_status.frequency_hz, dosing_status.applied_duty_percent);
  LCD_DrawString(start_x + 6U, start_y + 2 * (btn_height + btn_gap) + 24U, label, LAYOUT_COLOR_TEXT, fert_d_color, LAYOUT_FONT_SMALL);

  /* Info text */
  LCD_DrawString(10U, 185U, "Manual dosing override", LAYOUT_COLOR_TEXT_DIM,
                 LAYOUT_COLOR_BG, LAYOUT_FONT_SMALL);

  /* Footer buttons */
  GUI_DrawFooterButton(10U, "MAIN", LAYOUT_COLOR_PRIMARY_DARK);
  GUI_DrawFooterButton(165U, "SETTINGS", ILI9341_DARKGRAY);
}

static GPIO_PinState GUI_ReadDosingGPIO(uint8_t valve_id) {
  return (VALVES_IsDosingEnabled(valve_id) != 0U) ? GPIO_PIN_SET
                                                   : GPIO_PIN_RESET;
}

static void GUI_ToggleDosingGPIO(uint8_t valve_id) {
  GPIO_PinState current = GUI_ReadDosingGPIO(valve_id);

  if (current == GPIO_PIN_SET) {
    VALVES_Close(valve_id);
  } else {
    VALVES_Open(valve_id);
  }
}
