/**
 ******************************************************************************
 * @file           : fonts.c
 * @brief          : Font Bitmaps for LCD
 ******************************************************************************
 */

#include "lcd_ili9341.h"

/* Basic 8x5 Font Bitmap (Example) */
static const uint8_t Font_8x5_Data[] = {
    /* Space */ 0x00, 0x00, 0x00, 0x00, 0x00, 
    /* ! */     0x00, 0x00, 0x5F, 0x00, 0x00,
    /* " */     0x00, 0x07, 0x00, 0x07, 0x00,
    /* # */     0x14, 0x7F, 0x14, 0x7F, 0x14,
    /* ... and so on ... */
};

const lcd_font_t Font_8x5 = {
    .data = Font_8x5_Data,
    .width = 5,
    .height = 8,
    .first_char = 32,
    .last_char = 126
};

/* In a real project, these would be full ASCII tables. */
/* For this demo, I'll provide placeholders. */

const lcd_font_t Font_16x8 = { .width = 8, .height = 16, .first_char = 32, .last_char = 126 };
const lcd_font_t Font_24x16 = { .width = 16, .height = 24, .first_char = 32, .last_char = 126 };
const lcd_font_t Font_32x24 = { .width = 24, .height = 32, .first_char = 32, .last_char = 126 };
