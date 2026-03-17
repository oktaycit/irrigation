/**
 ******************************************************************************
 * @file           : lcd_ili9341.c
 * @brief          : ILI9341 TFT LCD 3.2" FSMC Driver Implementation
 ******************************************************************************
 */

#include "lcd_ili9341.h"
#include <stdlib.h>

extern TIM_HandleTypeDef htim1;

/* Orientation Private Variable */
static lcd_orientation_t current_orientation = LCD_ORIENTATION_PORTRAIT;

typedef struct {
  char character;
  uint8_t rows[7];
} lcd_fallback_glyph_t;

static const lcd_fallback_glyph_t g_lcd_fallback_font[] = {
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'+', {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}},
    {'-', {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}},
    {':', {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00}},
    {'/', {0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00}},
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}},
    {'6', {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E}},
    {'?', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}},
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'J', {0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'Q', {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}},
    {'X', {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
};

static const uint8_t *LCD_GetFallbackGlyph(char c);
static void LCD_DrawFallbackChar(uint16_t x, uint16_t y, char c,
                                 lcd_color_t fg, lcd_color_t bg,
                                 const lcd_font_t *font);

uint16_t LCD_GetDisplayWidth(void) {
  if (current_orientation == LCD_ORIENTATION_LANDSCAPE ||
      current_orientation == LCD_ORIENTATION_LANDSCAPE_FLIP) {
    return LCD_HEIGHT;
  }

  return LCD_WIDTH;
}

uint16_t LCD_GetDisplayHeight(void) {
  if (current_orientation == LCD_ORIENTATION_LANDSCAPE ||
      current_orientation == LCD_ORIENTATION_LANDSCAPE_FLIP) {
    return LCD_WIDTH;
  }

  return LCD_HEIGHT;
}

/**
 * @brief  LCD Reset function
 */
void LCD_Reset(void) {
  HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET);
  HAL_Delay(50);
  HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);
  HAL_Delay(120);
}

/**
 * @brief  LCD Software Initialization
 */
void LCD_Init(void) {
  /* Reset LCD anyway */
  LCD_Reset();

  /* Software Reset */
  LCD_WriteCommand(0x01);
  HAL_Delay(10);

  /* Power Control A */
  LCD_WriteCommand(0xCB);
  LCD_WriteData(0x39);
  LCD_WriteData(0x2C);
  LCD_WriteData(0x00);
  LCD_WriteData(0x34);
  LCD_WriteData(0x02);

  /* Power Control B */
  LCD_WriteCommand(0xCF);
  LCD_WriteData(0x00);
  LCD_WriteData(0xC1);
  LCD_WriteData(0x30);

  /* Driver Timing Control A */
  LCD_WriteCommand(0xE8);
  LCD_WriteData(0x85);
  LCD_WriteData(0x00);
  LCD_WriteData(0x78);

  /* Driver Timing Control B */
  LCD_WriteCommand(0xEA);
  LCD_WriteData(0x00);
  LCD_WriteData(0x00);

  /* Power On Sequence Control */
  LCD_WriteCommand(0xED);
  LCD_WriteData(0x64);
  LCD_WriteData(0x03);
  LCD_WriteData(0x12);
  LCD_WriteData(0x81);

  /* Pump Ratio Control */
  LCD_WriteCommand(0xF7);
  LCD_WriteData(0x20);

  /* Power Control 1 */
  LCD_WriteCommand(0xC0);
  LCD_WriteData(0x23);

  /* Power Control 2 */
  LCD_WriteCommand(0xC1);
  LCD_WriteData(0x10);

  /* VCOM Control 1 */
  LCD_WriteCommand(0xC5);
  LCD_WriteData(0x3E);
  LCD_WriteData(0x28);

  /* VCOM Control 2 */
  LCD_WriteCommand(0xC7);
  LCD_WriteData(0x86);

  /* Memory Access Control */
  LCD_WriteCommand(0x36);
  LCD_WriteData(0x48);

  /* Pixel Format Set */
  LCD_WriteCommand(0x3A);
  LCD_WriteData(0x55); /* 16-bit color */

  /* Frame Rate Control */
  LCD_WriteCommand(0xB1);
  LCD_WriteData(0x00);
  LCD_WriteData(0x18);

  /* Display Function Control */
  LCD_WriteCommand(0xB6);
  LCD_WriteData(0x08);
  LCD_WriteData(0x82);
  LCD_WriteData(0x27);

  /* 3Gamma Function Disable */
  LCD_WriteCommand(0xF2);
  LCD_WriteData(0x00);

  /* Gamma Curve Selected */
  LCD_WriteCommand(0x26);
  LCD_WriteData(0x01);

  /* Positive Gamma Correction */
  LCD_WriteCommand(0xE0);
  LCD_WriteData(0x0F);
  LCD_WriteData(0x31);
  LCD_WriteData(0x2B);
  LCD_WriteData(0x0C);
  LCD_WriteData(0x0E);
  LCD_WriteData(0x08);
  LCD_WriteData(0x4E);
  LCD_WriteData(0xF1);
  LCD_WriteData(0x37);
  LCD_WriteData(0x07);
  LCD_WriteData(0x10);
  LCD_WriteData(0x03);
  LCD_WriteData(0x0E);
  LCD_WriteData(0x09);
  LCD_WriteData(0x00);

  /* Negative Gamma Correction */
  LCD_WriteCommand(0xE1);
  LCD_WriteData(0x00);
  LCD_WriteData(0x0E);
  LCD_WriteData(0x14);
  LCD_WriteData(0x03);
  LCD_WriteData(0x11);
  LCD_WriteData(0x07);
  LCD_WriteData(0x31);
  LCD_WriteData(0xC1);
  LCD_WriteData(0x48);
  LCD_WriteData(0x08);
  LCD_WriteData(0x0F);
  LCD_WriteData(0x0C);
  LCD_WriteData(0x31);
  LCD_WriteData(0x36);
  LCD_WriteData(0x0F);

  /* Exit Sleep */
  LCD_WriteCommand(0x11);
  HAL_Delay(120);

  /* Display On */
  LCD_WriteCommand(0x29);

  /* Set default orientation */
  LCD_SetOrientation(LCD_ORIENTATION_PORTRAIT);

  /* Clear screen to black */
  LCD_Clear(ILI9341_BLACK);
}

/**
 * @brief  Set LCD orientation
 */
void LCD_SetOrientation(lcd_orientation_t orientation) {
  LCD_WriteCommand(ILI9341_MADCTL);
  current_orientation = orientation;

  switch (orientation) {
  case LCD_ORIENTATION_PORTRAIT:
    LCD_WriteData(MADCTL_MX | MADCTL_BGR);
    break;
  case LCD_ORIENTATION_LANDSCAPE:
    LCD_WriteData(MADCTL_MV | MADCTL_BGR);
    break;
  case LCD_ORIENTATION_PORTRAIT_FLIP:
    LCD_WriteData(MADCTL_MY | MADCTL_BGR);
    break;
  case LCD_ORIENTATION_LANDSCAPE_FLIP:
    LCD_WriteData(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
    break;
  }
}

/**
 * @brief  Clear LCD with specific color
 */
void LCD_Clear(lcd_color_t color) {
  uint32_t i;
  uint32_t pixels = LCD_PIXELS;

  LCD_SetCursor(0, 0);
  LCD_WriteCommand(ILI9341_RAMWR);

  for (i = 0; i < pixels; i++) {
    LCD_WriteData16(color);
  }
}

/**
 * @brief  Set LCD Address Window
 */
void LCD_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  LCD_WriteCommand(ILI9341_CASET);
  LCD_WriteData16(x0 >> 8);
  LCD_WriteData16(x0 & 0xFF);
  LCD_WriteData16(x1 >> 8);
  LCD_WriteData16(x1 & 0xFF);

  LCD_WriteCommand(ILI9341_RASET);
  LCD_WriteData16(y0 >> 8);
  LCD_WriteData16(y0 & 0xFF);
  LCD_WriteData16(y1 >> 8);
  LCD_WriteData16(y1 & 0xFF);
}

/**
 * @brief  Set Cursor Position
 */
void LCD_SetCursor(uint16_t x, uint16_t y) {
  LCD_SetAddressWindow(x, y, LCD_GetDisplayWidth() - 1U,
                       LCD_GetDisplayHeight() - 1U);
}

/**
 * @brief  Draw Pixel
 */
void LCD_DrawPixel(uint16_t x, uint16_t y, lcd_color_t color) {
  if (x >= LCD_GetDisplayWidth() || y >= LCD_GetDisplayHeight()) return;

  LCD_SetAddressWindow(x, y, x, y);
  LCD_WriteCommand(ILI9341_RAMWR);
  LCD_WriteData16(color);
}

/**
 * @brief  Fill Rectangle
 */
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                  lcd_color_t color) {
  uint32_t i;
  uint32_t pixels = (uint32_t)w * h;

  LCD_SetAddressWindow(x, y, x + w - 1, y + h - 1);
  LCD_WriteCommand(ILI9341_RAMWR);

  for (i = 0; i < pixels; i++) {
    LCD_WriteData16(color);
  }
}

/**
 * @brief  Draw Horizontal Line
 */
void LCD_DrawHLine(uint16_t x, uint16_t y, uint16_t w, lcd_color_t color) {
  LCD_FillRect(x, y, w, 1, color);
}

/**
 * @brief  Draw Vertical Line
 */
void LCD_DrawVLine(uint16_t x, uint16_t y, uint16_t h, lcd_color_t color) {
  LCD_FillRect(x, y, 1, h, color);
}

/**
 * @brief  Draw Rectangle Border
 */
void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                  lcd_color_t color) {
  LCD_DrawHLine(x, y, w, color);
  LCD_DrawHLine(x, y + h - 1, w, color);
  LCD_DrawVLine(x, y, h, color);
  LCD_DrawVLine(x + w - 1, y, h, color);
}

/**
 * @brief  Draw Line (Bresenham)
 */
void LCD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  lcd_color_t color) {
  int16_t dx = abs(x1 - x0);
  int16_t dy = -abs(y1 - y0);
  int16_t sx = (x0 < x1) ? 1 : -1;
  int16_t sy = (y0 < y1) ? 1 : -1;
  int16_t err = dx + dy;
  int16_t e2;

  while (1) {
    LCD_DrawPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

/**
 * @brief  Draw Char
 */
void LCD_DrawChar(uint16_t x, uint16_t y, char c, lcd_color_t fg,
                  lcd_color_t bg, const lcd_font_t *font) {
  LCD_DrawFallbackChar(x, y, c, fg, bg, font);
}

/**
 * @brief  Draw String
 */
void LCD_DrawString(uint16_t x, uint16_t y, const char *str, lcd_color_t fg,
                    lcd_color_t bg, const lcd_font_t *font) {
  while (*str) {
    LCD_DrawChar(x, y, *str++, fg, bg, font);
    x += (font != NULL && font->width > 0U) ? font->width : 6U;
  }
}

static const uint8_t *LCD_GetFallbackGlyph(char c) {
  size_t i;

  if (c >= 'a' && c <= 'z') {
    c = (char)(c - ('a' - 'A'));
  }

  for (i = 0; i < (sizeof(g_lcd_fallback_font) / sizeof(g_lcd_fallback_font[0]));
       i++) {
    if (g_lcd_fallback_font[i].character == c) {
      return g_lcd_fallback_font[i].rows;
    }
  }

  for (i = 0; i < (sizeof(g_lcd_fallback_font) / sizeof(g_lcd_fallback_font[0]));
       i++) {
    if (g_lcd_fallback_font[i].character == '?') {
      return g_lcd_fallback_font[i].rows;
    }
  }

  return g_lcd_fallback_font[0].rows;
}

static void LCD_DrawFallbackChar(uint16_t x, uint16_t y, char c,
                                 lcd_color_t fg, lcd_color_t bg,
                                 const lcd_font_t *font) {
  uint16_t scale_x = 1U;
  uint16_t scale_y = 1U;
  uint16_t char_w;
  uint16_t char_h;
  uint16_t row;
  uint16_t col;
  uint16_t dx;
  uint16_t dy;
  const uint8_t *glyph;

  if (font != NULL) {
    if (font->width >= 10U) {
      scale_x = 2U;
    }
    if (font->height >= 16U) {
      scale_y = 2U;
    }
    if (font->width >= 18U) {
      scale_x = 3U;
    }
    if (font->height >= 24U) {
      scale_y = 3U;
    }
  }

  char_w = 5U * scale_x;
  char_h = 7U * scale_y;
  glyph = LCD_GetFallbackGlyph(c);

  if (fg != bg) {
    for (row = 0; row < char_h; row++) {
      for (col = 0; col < char_w; col++) {
        LCD_DrawPixel(x + col, y + row, bg);
      }
    }
  }

  for (row = 0; row < 7U; row++) {
    for (col = 0; col < 5U; col++) {
      if ((glyph[row] & (1U << (4U - col))) == 0U) continue;

      for (dy = 0; dy < scale_y; dy++) {
        for (dx = 0; dx < scale_x; dx++) {
          LCD_DrawPixel(x + (col * scale_x) + dx, y + (row * scale_y) + dy, fg);
        }
      }
    }
  }
}

/**
 * @brief  Set Backlight Brightness
 */
void LCD_SetBacklight(uint8_t brightness) {
  if (brightness > 100) brightness = 100;
  /* TIM1_CH1 is used for PWM. Pulse value should be calculated based on Period */
  /* Assuming Period is 1000 for 0-100 percentage mapping */
  uint32_t pulse = (brightness * (uint32_t)__HAL_TIM_GET_AUTORELOAD(&htim1)) / 100;
  __HAL_TIM_SET_COMPARE(&htim1, LCD_BL_TIM_CHANNEL, pulse);
}

/**
 * @brief  Sleep Mode
 */
void LCD_Sleep(void) {
  LCD_WriteCommand(0x28); /* Display OFF */
  HAL_Delay(5);
  LCD_WriteCommand(0x10); /* Sleep ON */
}

/**
 * @brief  Wake Up from Sleep
 */
void LCD_WakeUp(void) {
  LCD_WriteCommand(0x11); /* Sleep OFF */
  HAL_Delay(120);
  LCD_WriteCommand(0x29); /* Display ON */
}

/* Helper for DrawCircle, FillCircle, DrawTriangle, etc. would go here */
/* For brevity, minimal implementation provided. */
void LCD_DrawCircle(uint16_t x, uint16_t y, uint16_t r, lcd_color_t color) { /* Implementation */ }
void LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r, lcd_color_t color) { /* Implementation */ }
void LCD_DrawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, lcd_color_t color) { /* Implementation */ }
void LCD_FillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, lcd_color_t color) { /* Implementation */ }
void LCD_DrawBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const lcd_color_t *bitmap) {
    LCD_SetAddressWindow(x, y, x + w - 1, y + h - 1);
    LCD_WriteCommand(ILI9341_RAMWR);
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) {
        LCD_WriteData16(bitmap[i]);
    }
}
