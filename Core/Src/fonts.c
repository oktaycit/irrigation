/**
 ******************************************************************************
 * @file           : fonts.c
 * @brief          : Font Bitmaps for LCD (IL9341)
 ******************************************************************************
 */

#include "lcd_ili9341.h"

/* 16x8 Font - Simple 8x16 pixel ASCII font */
static const uint8_t Font_16x8_Data[95][16] = {
    {0} /* Space - placeholder, repeat for all 95 ASCII chars (32-126) */
    /* Full font table would go here - using placeholder for now */
};

const lcd_font_t Font_16x8 = {
    .data = (const uint8_t *)Font_16x8_Data,
    .width = 8,
    .height = 16,
    .first_char = 32,
    .last_char = 126
};

/* 24x16 Font */
static const uint8_t Font_24x16_Data[95][24] = {
    {0} /* Placeholder */
};

const lcd_font_t Font_24x16 = {
    .data = (const uint8_t *)Font_24x16_Data,
    .width = 16,
    .height = 24,
    .first_char = 32,
    .last_char = 126
};

/* 32x24 Font */
static const uint8_t Font_32x24_Data[95][32] = {
    {0} /* Placeholder */
};

const lcd_font_t Font_32x24 = {
    .data = (const uint8_t *)Font_32x24_Data,
    .width = 24,
    .height = 32,
    .first_char = 32,
    .last_char = 126
};

/* 8x5 Font */
static const uint8_t Font_8x5_Data[95][5] = {
    {0} /* Placeholder */
};

const lcd_font_t Font_8x5 = {
    .data = (const uint8_t *)Font_8x5_Data,
    .width = 5,
    .height = 8,
    .first_char = 32,
    .last_char = 126
};
