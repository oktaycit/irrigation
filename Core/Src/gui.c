/**
 ******************************************************************************
 * @file           : gui.c
 * @brief          : Grafiksel Kullanıcı Arayüzü (GUI) Driver Implementation
 ******************************************************************************
 */

#include "main.h"
#include <stdio.h>
#include <string.h>

#define GUI_REFRESH_MS 300U
#define GUI_HEADER_HEIGHT 30U
#define GUI_TOUCH_DEBOUNCE_MS 120U

typedef struct {
  ph_control_params_t ph_draft;
  ec_control_params_t ec_draft;
  uint8_t selected_parcel;
  uint32_t parcel_duration_sec;
  uint8_t parcel_enabled;
  char notice[32];
} gui_runtime_state_t;

/* Internal Handle */
static gui_handle_t h_gui = {0};
static gui_runtime_state_t g_gui_state = {0};

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
static void GUI_DrawSystemInfoScreen(void);
static void GUI_DrawBackButton(void);
static void GUI_DrawFooterButton(uint16_t x, const char *text,
                                 lcd_color_t color);
static void GUI_DrawAdjustRow(uint16_t y, const char *label, float value,
                              const char *unit);
static void GUI_DrawParcelAdjustRow(const char *label, const char *value,
                                    uint16_t y);
static void GUI_SetNotice(const char *text);
static const char *GUI_GetSystemHealthText(void);
static const char *GUI_GetAutoModeName(auto_mode_t mode);
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

/**
 * @brief  Initialize GUI
 */
void GUI_Init(void) {
  memset(&h_gui, 0, sizeof(gui_handle_t));
  memset(&g_gui_state, 0, sizeof(gui_runtime_state_t));

  LCD_SetOrientation(LCD_ORIENTATION_LANDSCAPE);
  LCD_Clear(ILI9341_BLACK);

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
  static uint32_t last_update = 0U;
  touch_point_t point = {0};

  if (h_gui.is_initialized == 0U) return;

  if (TOUCH_ReadPoint(&point) != 0U) {
    if ((h_gui.last_touch.pressed == 0U) &&
        ((HAL_GetTick() - h_gui.last_touch_time) >= GUI_TOUCH_DEBOUNCE_MS)) {
      GUI_ProcessTouch(&point);
      h_gui.last_touch_time = HAL_GetTick();
    }
    h_gui.last_touch = point;
  } else {
    memset(&h_gui.last_touch, 0, sizeof(h_gui.last_touch));
  }

  if ((HAL_GetTick() - last_update) < GUI_REFRESH_MS) return;
  last_update = HAL_GetTick();

  switch (h_gui.current_screen) {
  case SCREEN_MAIN:
  case SCREEN_MANUAL:
  case SCREEN_SYSTEM_INFO:
    GUI_Redraw();
    break;
  default:
    break;
  }
}

/**
 * @brief  Navigate to a specific screen
 */
void GUI_NavigateTo(screen_id_t screen_id) {
  if (screen_id >= SCREEN_MAX) return;

  GUI_LoadScreenState(screen_id);
  h_gui.current_screen = screen_id;
  LCD_Clear(ILI9341_BLACK);
  GUI_DrawScreen(screen_id);
}

/**
 * @brief  Draw Main Screen Layout
 */
void GUI_DrawMainScreen(void) {
  char text[32];
  uint8_t current_parcel = IRRIGATION_CTRL_GetCurrentParcelId();
  uint32_t remaining_sec = IRRIGATION_CTRL_GetRemainingTime();
  uint16_t screen_width = LCD_GetDisplayWidth();

  LCD_Clear(ILI9341_BLACK);

  GUI_DrawStatusbar(IRRIGATION_CTRL_GetStateName(IRRIGATION_CTRL_GetState()),
                    GUI_GetSystemHealthText());

  GUI_DrawValueBox(10U, 42U, 145U, 56U, "PH", IRRIGATION_CTRL_GetPH(), "",
                   ILI9341_CYAN, ILI9341_BLACK);
  GUI_DrawValueBox(165U, 42U, 145U, 56U, "EC", IRRIGATION_CTRL_GetEC(), "MS",
                   ILI9341_GREEN, ILI9341_BLACK);

  snprintf(text, sizeof(text), "MODE %s",
           GUI_GetAutoModeName(IRRIGATION_CTRL_GetAutoMode()));
  LCD_DrawString(10U, 108U, text, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);

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
  default:
    GUI_DrawHeader("MENU");
    break;
  }
}

screen_id_t GUI_GetCurrentScreen(void) {
  return (screen_id_t)h_gui.current_screen;
}

void GUI_Redraw(void) {
  LCD_Clear(ILI9341_BLACK);
  GUI_DrawScreen((screen_id_t)h_gui.current_screen);
}

void GUI_DrawValueBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                      const char *label, float value, const char *unit,
                      lcd_color_t fg, lcd_color_t bg) {
  char value_text[32];

  LCD_DrawRect(x, y, w, h, fg);
  LCD_DrawString(x + 8U, y + 8U, label, fg, bg, &Font_16x8);

  if (unit != NULL && unit[0] != '\0') {
    snprintf(value_text, sizeof(value_text), "%.2f %s", value, unit);
  } else {
    snprintf(value_text, sizeof(value_text), "%.2f", value);
  }

  LCD_DrawString(x + 8U, y + 30U, value_text, ILI9341_WHITE, bg, &Font_16x8);
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
  (void)ph;
  if (h_gui.current_screen == SCREEN_MAIN) {
    GUI_DrawMainScreen();
  }
}

void GUI_UpdateECValue(float ec) {
  (void)ec;
  if (h_gui.current_screen == SCREEN_MAIN) {
    GUI_DrawMainScreen();
  }
}

void GUI_UpdateValveStatus(uint8_t valve_id, uint8_t is_open) {
  (void)valve_id;
  (void)is_open;
  if (h_gui.current_screen == SCREEN_MANUAL) {
    GUI_DrawManualScreen();
  }
}

void GUI_UpdateIrrigationStatus(uint8_t is_running, uint8_t parcel_id) {
  (void)is_running;
  (void)parcel_id;
  if (h_gui.current_screen == SCREEN_MAIN) {
    GUI_DrawMainScreen();
  }
}

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
  char label[20];

  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("MANUAL VALVES");
  GUI_DrawBackButton();

  for (uint8_t valve_id = 1U; valve_id <= VALVE_COUNT; valve_id++) {
    uint16_t x = (valve_id % 2U == 1U) ? 10U : 165U;
    uint16_t y = 42U + (uint16_t)(((valve_id - 1U) / 2U) * 39U);
    uint8_t is_open = (VALVES_GetState(valve_id) == VALVE_STATE_OPEN);

    snprintf(label, sizeof(label), "V%u %s", valve_id,
             (is_open != 0U) ? "OPEN" : "CLOSED");
    GUI_DrawButton(x, y, 145U, 32U, label,
                   (is_open != 0U) ? ILI9341_DARKGREEN : ILI9341_DARKGRAY);
  }

  GUI_DrawFooterButton(10U, "ALL OFF", ILI9341_DARKRED);
  GUI_DrawFooterButton(165U, "MAIN", ILI9341_NAVY);
}

static void GUI_DrawSettingsScreen(void) {
  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("SETTINGS");
  GUI_DrawBackButton();

  GUI_DrawButton(10U, 45U, 145U, 42U, "PH", ILI9341_DARKGRAY);
  GUI_DrawButton(165U, 45U, 145U, 42U, "EC", ILI9341_DARKGRAY);
  GUI_DrawButton(10U, 95U, 145U, 42U, "PARCEL", ILI9341_DARKGRAY);
  GUI_DrawButton(165U, 95U, 145U, 42U, "CALIB", ILI9341_DARKGRAY);
  GUI_DrawButton(10U, 145U, 145U, 42U, "INFO", ILI9341_DARKGRAY);
  GUI_DrawButton(165U, 145U, 145U, 42U, "MAIN", ILI9341_NAVY);
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

static void GUI_DrawSystemInfoScreen(void) {
  char text[40];
  uint16_t y = 42U;

  LCD_Clear(ILI9341_BLACK);
  GUI_DrawHeader("SYSTEM INFO");
  GUI_DrawBackButton();

  snprintf(text, sizeof(text), "FW %s", FIRMWARE_VERSION);
  LCD_DrawString(10U, y, text, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);
  y += 22U;

  snprintf(text, sizeof(text), "STATE %s",
           IRRIGATION_CTRL_GetStateName(IRRIGATION_CTRL_GetState()));
  LCD_DrawString(10U, y, text, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);
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

static void GUI_SetNotice(const char *text) {
  snprintf(g_gui_state.notice, sizeof(g_gui_state.notice), "%s", text);
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
    GUI_Redraw();
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
    GUI_Redraw();
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

    GUI_Redraw();
    return;
  }
}

static void GUI_HandleSettingsTouch(const touch_point_t *point) {
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, 4U, 64U, 22U) != 0U) {
    GUI_NavigateTo(SCREEN_MAIN);
    return;
  }

  if (GUI_PointInRect(point, 10U, 45U, 145U, 42U) != 0U) {
    GUI_NavigateTo(SCREEN_PH_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 165U, 45U, 145U, 42U) != 0U) {
    GUI_NavigateTo(SCREEN_EC_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 10U, 95U, 145U, 42U) != 0U) {
    GUI_NavigateTo(SCREEN_PARCEL_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 165U, 95U, 145U, 42U) != 0U) {
    GUI_NavigateTo(SCREEN_CALIBRATION);
    return;
  }

  if (GUI_PointInRect(point, 10U, 145U, 145U, 42U) != 0U) {
    GUI_NavigateTo(SCREEN_SYSTEM_INFO);
    return;
  }

  if (GUI_PointInRect(point, 165U, 145U, 145U, 42U) != 0U) {
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
  if (GUI_PointInRect(point, LCD_GetDisplayWidth() - 72U, 4U, 64U, 22U) != 0U) {
    GUI_NavigateTo(SCREEN_SETTINGS);
    return;
  }

  if (GUI_PointInRect(point, 10U, 48U, 145U, 42U) != 0U) {
    TOUCH_CalibrationStart();
    GUI_SetNotice("TOUCH CAL START");
    GUI_DrawCalibrationScreen();
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
