/**
 ******************************************************************************
 * @file           : lcd_ili9341.h
 * @brief          : ILI9341 TFT LCD 3.2" FSMC Driver
 ******************************************************************************
 */

#ifndef __LCD_ILI9341_H
#define __LCD_ILI9341_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

/* LCD Configuration --------------------------------------------------------*/
#define LCD_WIDTH 240U
#define LCD_HEIGHT 320U
#define LCD_PIXELS (LCD_WIDTH * LCD_HEIGHT)

/* Color Definitions --------------------------------------------------------*/
#define ILI9341_BLACK 0x0000U
#define ILI9341_BLUE 0x001FU
#define ILI9341_RED 0xF800U
#define ILI9341_GREEN 0x07E0U
#define ILI9341_CYAN 0x07FFU
#define ILI9341_MAGENTA 0xF81FU
#define ILI9341_YELLOW 0xFFE0U
#define ILI9341_WHITE 0xFFFFU
#define ILI9341_GRAY 0x8410U
#define ILI9341_DARKGRAY 0x4208U
#define ILI9341_NAVY 0x000FU
#define ILI9341_DARKGREEN 0x03E0U
#define ILI9341_DARKRED 0x8000U
#define ILI9341_ORANGE 0xFD20U
#define ILI9341_PINK 0xF81FU
#define ILI9341_BROWN 0xBC40U

/* ILI9341 Commands ---------------------------------------------------------*/
#define ILI9341_NOP 0x00U
#define ILI9341_SWRESET 0x01U
#define ILI9341_RDDID 0x04U
#define ILI9341_RDDST 0x09U
#define ILI9341_SLPIN 0x10U
#define ILI9341_SLPOUT 0x11U
#define ILI9341_PTLON 0x12U
#define ILI9341_NORON 0x13U
#define ILI9341_RDMODE 0x0AU
#define ILI9341_RDMADCTL 0x0BU
#define ILI9341_RDPIXFMT 0x0CU
#define ILI9341_RDIMGFMT 0x0DU
#define ILI9341_RDSELFDIAG 0x0FU
#define ILI9341_RAMRD 0x2EU
#define ILI9341_CASET 0x2AU
#define ILI9341_RASET 0x2BU
#define ILI9341_RAMWR 0x2CU
#define ILI9341_RAMRD 0x2EU
#define ILI9341_PTLAR 0x30U
#define ILI9341_MADCTL 0x36U
#define ILI9341_VSCRSADD 0x37U
#define ILI9341_PIXFMT 0x3AU
#define ILI9341_FRMCTR1 0xB1U
#define ILI9341_FRMCTR2 0xB2U
#define ILI9341_FRMCTR3 0xB3U
#define ILI9341_INVCTR 0xB4U
#define ILI9341_DFUNCTR 0xB6U
#define ILI9341_PWCTR1 0xC0U
#define ILI9341_PWCTR2 0xC1U
#define ILI9341_PWCTR3 0xC2U
#define ILI9341_PWCTR4 0xC3U
#define ILI9341_PWCTR5 0xC4U
#define ILI9341_VMCTR1 0xC5U
#define ILI9341_VMCTR2 0xC7U
#define ILI9341_RDID1 0xDAU
#define ILI9341_RDID2 0xDBU
#define ILI9341_RDID3 0xDCU
#define ILI9341_RDID4 0xDDU
#define ILI9341_GMCTRP1 0xE0U
#define ILI9341_GMCTRN1 0xE1U

/* MADCTL Bits -------------------------------------------------------------*/
#define MADCTL_MY 0x80U  /* Page Address Order */
#define MADCTL_MX 0x40U  /* Column Address Order */
#define MADCTL_MV 0x20U  /* Page/Column Order */
#define MADCTL_ML 0x10U  /* Line Address Order */
#define MADCTL_RGB 0x00U /* RGB Order */
#define MADCTL_BGR 0x08U /* BGR Order */
#define MADCTL_MH 0x04U  /* Display Data Latch Order */

/* FSMC Bank Address --------------------------------------------------------*/
#define LCD_BASE_ADDR 0x60000000U
#define LCD_CMD_REG (*(volatile uint16_t *)(LCD_BASE_ADDR + 0x00000000U))
/*
 * The J1 TFT header routes the LCD RS/DC line to FSMC_A18. On a 16-bit FSMC
 * bus that address line appears at byte offset 1 << (18 + 1) = 0x00080000.
 */
#define LCD_DATA_REG (*(volatile uint16_t *)(LCD_BASE_ADDR + 0x00080000U))

/* GPIO Pin Definitions -----------------------------------------------------*/
/*
 * On the STM32 F4VE J1 TFT header the panel reset line is tied to NRST,
 * so the display resets together with the MCU and there is no dedicated
 * GPIO-driven LCD reset signal available to firmware.
 */
#define LCD_BL_PIN GPIO_PIN_1
#define LCD_BL_PORT GPIOB

/* LCD Data Type -----------------------------------------------------------*/
typedef uint16_t lcd_color_t;

/* LCD Orientation ---------------------------------------------------------*/
typedef enum {
  LCD_ORIENTATION_PORTRAIT = 0,
  LCD_ORIENTATION_LANDSCAPE,
  LCD_ORIENTATION_PORTRAIT_FLIP,
  LCD_ORIENTATION_LANDSCAPE_FLIP
} lcd_orientation_t;

/* Font Structure ----------------------------------------------------------*/
typedef struct {
  const uint8_t *data;
  uint16_t width;
  uint16_t height;
  uint8_t first_char;
  uint8_t last_char;
} lcd_font_t;

/* LCD Functions -----------------------------------------------------------*/
void LCD_Init(void);
void LCD_Reset(void);
void LCD_SetOrientation(lcd_orientation_t orientation);
uint16_t LCD_GetDisplayWidth(void);
uint16_t LCD_GetDisplayHeight(void);
void LCD_Clear(lcd_color_t color);
void LCD_SetCursor(uint16_t x, uint16_t y);
void LCD_DrawPixel(uint16_t x, uint16_t y, lcd_color_t color);
void LCD_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                  lcd_color_t color);
void LCD_DrawHLine(uint16_t x, uint16_t y, uint16_t length, lcd_color_t color);
void LCD_DrawVLine(uint16_t x, uint16_t y, uint16_t length, lcd_color_t color);
void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                  lcd_color_t color);
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                  lcd_color_t color);
void LCD_DrawCircle(uint16_t x, uint16_t y, uint16_t r, lcd_color_t color);
void LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r, lcd_color_t color);
void LCD_DrawTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                      uint16_t x2, uint16_t y2, lcd_color_t color);
void LCD_FillTriangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                      uint16_t x2, uint16_t y2, lcd_color_t color);
void LCD_DrawChar(uint16_t x, uint16_t y, char c, lcd_color_t fg,
                  lcd_color_t bg, const lcd_font_t *font);
void LCD_DrawString(uint16_t x, uint16_t y, const char *str, lcd_color_t fg,
                    lcd_color_t bg, const lcd_font_t *font);
void LCD_DrawBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                    const lcd_color_t *bitmap);
void LCD_SetBacklight(uint8_t brightness); /* 0-100 */
void LCD_Sleep(void);
void LCD_WakeUp(void);

/* Inline Functions --------------------------------------------------------*/
static inline void LCD_WriteCommand(uint8_t cmd) {
  LCD_CMD_REG = (uint16_t)cmd;
  __asm volatile("nop");
  __asm volatile("nop");
}

static inline void LCD_WriteData(uint16_t data) {
  LCD_DATA_REG = data;
  __asm volatile("nop");
  __asm volatile("nop");
}

static inline void LCD_WriteData16(uint16_t data) { LCD_DATA_REG = data; }

static inline uint16_t LCD_ReadData16(void) {
  uint16_t data = LCD_DATA_REG;
  __asm volatile("nop");
  __asm volatile("nop");
  return data;
}

#ifdef __cplusplus
}
#endif

#endif /* __LCD_ILI9341_H */
