/**
 ******************************************************************************
 * @file           : touch_xpt2046.c
 * @brief          : XPT2046 Touch Screen Controller Driver Implementation
 ******************************************************************************
 */

#include "main.h"
#include <stdlib.h>
#include <string.h>

extern SPI_HandleTypeDef hspi2;

typedef struct {
  uint16_t raw_x;
  uint16_t raw_y;
} touch_raw_point_t;

static touch_calibration_t current_cal = {
    .x_min = 0,
    .x_max = 0,
    .y_min = 0,
    .y_max = 0,
    .swap_xy = 0,
    .invert_x = 0,
    .invert_y = 0,
    .valid = 0,
};

static struct {
  uint8_t active;
  uint8_t step;
  touch_raw_point_t top_left;
  touch_raw_point_t top_right;
  touch_raw_point_t bottom_left;
} g_touch_calibration = {0};

static uint16_t TOUCH_ReadRaw(uint8_t cmd);
static uint8_t TOUCH_ReadRawAvg(uint8_t cmd, uint16_t *val);
static int32_t TOUCH_Clamp(int32_t value, int32_t min_v, int32_t max_v);
static uint16_t TOUCH_MapRawAxis(uint16_t raw, int32_t raw_min, int32_t raw_max,
                                 uint16_t screen_max, uint8_t invert);
static void TOUCH_ApplyCalibration(uint16_t raw_x, uint16_t raw_y,
                                   uint16_t *screen_x, uint16_t *screen_y,
                                   uint16_t display_width,
                                   uint16_t display_height);
static void TOUCH_FinalizeCalibration(void);

void TOUCH_Init(void) {
  HAL_GPIO_WritePin(TOUCH_CS_PORT, TOUCH_CS_PIN, GPIO_PIN_SET);
  TOUCH_LoadCalibration();
}

uint8_t TOUCH_IsPressed(void) {
  return (HAL_GPIO_ReadPin(TOUCH_IRQ_PORT, TOUCH_IRQ_PIN) == GPIO_PIN_RESET);
}

static uint16_t TOUCH_ReadRaw(uint8_t cmd) {
  uint8_t tx_data[3] = {cmd, 0x00, 0x00};
  uint8_t rx_data[3] = {0};
  uint16_t value = 0;

  HAL_GPIO_WritePin(TOUCH_CS_PORT, TOUCH_CS_PIN, GPIO_PIN_RESET);
  HAL_SPI_TransmitReceive(&hspi2, tx_data, rx_data, 3, 100);
  HAL_GPIO_WritePin(TOUCH_CS_PORT, TOUCH_CS_PIN, GPIO_PIN_SET);

  value = ((uint16_t)rx_data[1] << 8) | rx_data[2];
  value >>= 3;

  return value;
}

static uint8_t TOUCH_ReadRawAvg(uint8_t cmd, uint16_t *val) {
  uint32_t sum = 0U;
  uint16_t min_v = 0xFFFFU;
  uint16_t max_v = 0U;
  uint16_t temp = 0U;
  const uint8_t samples = 12U;

  if (val == NULL) {
    return 0U;
  }

  for (uint8_t i = 0U; i < samples; i++) {
    temp = TOUCH_ReadRaw(cmd);
    if (temp < min_v) min_v = temp;
    if (temp > max_v) max_v = temp;
    sum += temp;
  }

  sum = sum - min_v - max_v;
  *val = (uint16_t)(sum / (samples - 2U));

  return 1U;
}

uint8_t TOUCH_ReadPoint(touch_point_t *point) {
  uint16_t raw_x = 0U;
  uint16_t raw_y = 0U;
  uint16_t display_width = 0U;
  uint16_t display_height = 0U;
  uint16_t screen_x = 0U;
  uint16_t screen_y = 0U;

  if (point == NULL) {
    return 0U;
  }

  if (!TOUCH_IsPressed()) {
    point->pressed = 0U;
    return 0U;
  }

  (void)TOUCH_ReadRawAvg(XPT2046_ADDR_X, &raw_x);
  (void)TOUCH_ReadRawAvg(XPT2046_ADDR_Y, &raw_y);

  point->raw_x = raw_x;
  point->raw_y = raw_y;
  point->pressed = 1U;
  display_width = LCD_GetDisplayWidth();
  display_height = LCD_GetDisplayHeight();

  if (current_cal.valid != 0U) {
    TOUCH_ApplyCalibration(raw_x, raw_y, &screen_x, &screen_y, display_width,
                           display_height);
    point->x = (int16_t)screen_x;
    point->y = (int16_t)screen_y;
  } else {
    point->x = (int16_t)(((uint32_t)raw_x * display_width) / 4096U);
    point->y = (int16_t)(((uint32_t)raw_y * display_height) / 4096U);
  }

  return 1U;
}

void TOUCH_GetPoint(touch_point_t *point) { (void)TOUCH_ReadPoint(point); }

void TOUCH_CalibrationStart(void) {
  memset(&g_touch_calibration, 0, sizeof(g_touch_calibration));
  g_touch_calibration.active = 1U;
  g_touch_calibration.step = 1U;
}

uint8_t TOUCH_CalibrationProcess(uint16_t *x, uint16_t *y, uint8_t *step) {
  if ((x == NULL) || (y == NULL)) {
    return 0U;
  }

  if (g_touch_calibration.active == 0U) {
    if (step != NULL) {
      *step = 0U;
    }
    return 0U;
  }

  switch (g_touch_calibration.step) {
  case 1U:
    g_touch_calibration.top_left.raw_x = *x;
    g_touch_calibration.top_left.raw_y = *y;
    g_touch_calibration.step = 2U;
    break;
  case 2U:
    g_touch_calibration.top_right.raw_x = *x;
    g_touch_calibration.top_right.raw_y = *y;
    g_touch_calibration.step = 3U;
    break;
  case 3U:
    g_touch_calibration.bottom_left.raw_x = *x;
    g_touch_calibration.bottom_left.raw_y = *y;
    TOUCH_FinalizeCalibration();
    break;
  default:
    break;
  }

  if (step != NULL) {
    *step = g_touch_calibration.step;
  }

  return (g_touch_calibration.active == 0U) ? 1U : 0U;
}

uint8_t TOUCH_IsCalibrating(void) { return g_touch_calibration.active; }

uint8_t TOUCH_GetCalibrationStep(void) { return g_touch_calibration.step; }

void TOUCH_SetCalibration(const touch_calibration_t *cal) {
  if (cal == NULL) {
    return;
  }
  current_cal = *cal;
}

void TOUCH_GetCalibration(touch_calibration_t *cal) {
  if (cal == NULL) {
    return;
  }
  *cal = current_cal;
}

void TOUCH_LoadCalibration(void) {
  touch_calibration_t stored = {0};

  if (EEPROM_LoadTouchCalibration(&stored) == EEPROM_OK) {
    current_cal = stored;
  } else {
    memset(&current_cal, 0, sizeof(current_cal));
  }
}

void TOUCH_SaveCalibration(void) {
  if (current_cal.valid == 0U) {
    return;
  }

  (void)EEPROM_SaveTouchCalibration(&current_cal);
}

void TOUCH_IRQHandler_Callback(void) {
  LOW_POWER_UpdateActivity();
}

void TOUCH_EnableIT(void) {
  HAL_NVIC_SetPriority(TOUCH_IRQ_EXTI, 0, 0);
  HAL_NVIC_EnableIRQ(TOUCH_IRQ_EXTI);
}

void TOUCH_DisableIT(void) { HAL_NVIC_DisableIRQ(TOUCH_IRQ_EXTI); }

static int32_t TOUCH_Clamp(int32_t value, int32_t min_v, int32_t max_v) {
  if (value < min_v) return min_v;
  if (value > max_v) return max_v;
  return value;
}

static uint16_t TOUCH_MapRawAxis(uint16_t raw, int32_t raw_min, int32_t raw_max,
                                 uint16_t screen_max, uint8_t invert) {
  int32_t clamped = 0;
  int32_t span = raw_max - raw_min;
  int32_t mapped = 0;

  if ((span <= 0) || (screen_max == 0U)) {
    return 0U;
  }

  clamped = TOUCH_Clamp((int32_t)raw, raw_min, raw_max);
  mapped = ((clamped - raw_min) * (int32_t)(screen_max - 1U)) / span;

  if (invert != 0U) {
    mapped = (int32_t)(screen_max - 1U) - mapped;
  }

  return (uint16_t)TOUCH_Clamp(mapped, 0, (int32_t)(screen_max - 1U));
}

static void TOUCH_ApplyCalibration(uint16_t raw_x, uint16_t raw_y,
                                   uint16_t *screen_x, uint16_t *screen_y,
                                   uint16_t display_width,
                                   uint16_t display_height) {
  uint16_t x_raw = raw_x;
  uint16_t y_raw = raw_y;

  if ((screen_x == NULL) || (screen_y == NULL)) {
    return;
  }

  if (current_cal.swap_xy != 0U) {
    x_raw = raw_y;
    y_raw = raw_x;
  }

  *screen_x = TOUCH_MapRawAxis(x_raw, current_cal.x_min, current_cal.x_max,
                               display_width, current_cal.invert_x);
  *screen_y = TOUCH_MapRawAxis(y_raw, current_cal.y_min, current_cal.y_max,
                               display_height, current_cal.invert_y);
}

static void TOUCH_FinalizeCalibration(void) {
  int32_t score_normal = 0;
  int32_t score_swapped = 0;
  uint8_t swap_xy = 0U;
  int32_t left_axis = 0;
  int32_t right_axis = 0;
  int32_t top_axis = 0;
  int32_t bottom_axis = 0;

  score_normal =
      abs((int32_t)g_touch_calibration.top_right.raw_x -
          (int32_t)g_touch_calibration.top_left.raw_x) +
      abs((int32_t)g_touch_calibration.bottom_left.raw_y -
          (int32_t)g_touch_calibration.top_left.raw_y);
  score_swapped =
      abs((int32_t)g_touch_calibration.top_right.raw_y -
          (int32_t)g_touch_calibration.top_left.raw_y) +
      abs((int32_t)g_touch_calibration.bottom_left.raw_x -
          (int32_t)g_touch_calibration.top_left.raw_x);

  swap_xy = (score_swapped > score_normal) ? 1U : 0U;

  if (swap_xy == 0U) {
    left_axis = ((int32_t)g_touch_calibration.top_left.raw_x +
                 (int32_t)g_touch_calibration.bottom_left.raw_x) /
                2;
    right_axis = (int32_t)g_touch_calibration.top_right.raw_x;
    top_axis = ((int32_t)g_touch_calibration.top_left.raw_y +
                (int32_t)g_touch_calibration.top_right.raw_y) /
               2;
    bottom_axis = (int32_t)g_touch_calibration.bottom_left.raw_y;
  } else {
    left_axis = ((int32_t)g_touch_calibration.top_left.raw_y +
                 (int32_t)g_touch_calibration.bottom_left.raw_y) /
                2;
    right_axis = (int32_t)g_touch_calibration.top_right.raw_y;
    top_axis = ((int32_t)g_touch_calibration.top_left.raw_x +
                (int32_t)g_touch_calibration.top_right.raw_x) /
               2;
    bottom_axis = (int32_t)g_touch_calibration.bottom_left.raw_x;
  }

  current_cal.swap_xy = swap_xy;
  current_cal.invert_x = (left_axis > right_axis) ? 1U : 0U;
  current_cal.invert_y = (top_axis > bottom_axis) ? 1U : 0U;

  if (left_axis < right_axis) {
    current_cal.x_min = left_axis;
    current_cal.x_max = right_axis;
  } else {
    current_cal.x_min = right_axis;
    current_cal.x_max = left_axis;
  }

  if (top_axis < bottom_axis) {
    current_cal.y_min = top_axis;
    current_cal.y_max = bottom_axis;
  } else {
    current_cal.y_min = bottom_axis;
    current_cal.y_max = top_axis;
  }

  current_cal.valid = 1U;
  g_touch_calibration.active = 0U;
  g_touch_calibration.step = 0U;
  TOUCH_SaveCalibration();
}
