/**
 ******************************************************************************
 * @file           : gui_layout.h
 * @brief          : GUI Layout Configuration - Centralized dimensions and spacing
 ******************************************************************************
 */

#ifndef __GUI_LAYOUT_H
#define __GUI_LAYOUT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "lcd_ili9341.h"

/* Display Dimensions (Landscape Mode) --------------------------------------*/
#define LAYOUT_SCREEN_WIDTH  320U  /* Landscape: swapped from 240 */
#define LAYOUT_SCREEN_HEIGHT 240U  /* Landscape: swapped from 320 */

/* Grid System --------------------------------------------------------------*/
#define LAYOUT_GRID_COLS     12U   /* 12-column grid for flexibility */
#define LAYOUT_GRID_ROWS     8U    /* 8-row grid for vertical layout */
#define LAYOUT_GRID_MARGIN   10U   /* Outer margin in pixels */
#define LAYOUT_GRID_GAP      8U    /* Gap between grid cells */

/* Calculated Grid Dimensions -----------------------------------------------*/
#define LAYOUT_GRID_CELL_W   ((LAYOUT_SCREEN_WIDTH - (2U * LAYOUT_GRID_MARGIN) - \
                               ((LAYOUT_GRID_COLS - 1U) * LAYOUT_GRID_GAP)) / LAYOUT_GRID_COLS)
#define LAYOUT_GRID_CELL_H   ((LAYOUT_SCREEN_HEIGHT - (2U * LAYOUT_GRID_MARGIN) - \
                               ((LAYOUT_GRID_ROWS - 1U) * LAYOUT_GRID_GAP)) / LAYOUT_GRID_ROWS)

/* Section Definitions ------------------------------------------------------*/
/* Header Area (En üstte) */
#define LAYOUT_HEADER_HEIGHT     24U
#define LAYOUT_HEADER_Y          0U

/* Status Bar (Header'ın hemen altında) */
#define LAYOUT_STATUSBAR_HEIGHT  24U
#define LAYOUT_STATUSBAR_Y       LAYOUT_HEADER_HEIGHT

/* Footer Area */
#define LAYOUT_FOOTER_HEIGHT     44U
#define LAYOUT_FOOTER_Y          (LAYOUT_SCREEN_HEIGHT - LAYOUT_FOOTER_HEIGHT)

/* Main Content Area */
#define LAYOUT_CONTENT_Y         (LAYOUT_HEADER_HEIGHT + LAYOUT_STATUSBAR_HEIGHT)
#define LAYOUT_CONTENT_HEIGHT    (LAYOUT_SCREEN_HEIGHT - LAYOUT_HEADER_HEIGHT - \
                                  LAYOUT_STATUSBAR_HEIGHT - LAYOUT_FOOTER_HEIGHT)

/* Value Boxes (Main Screen) ------------------------------------------------*/
#define LAYOUT_VALUE_BOX_MARGIN_TOP    8U
#define LAYOUT_VALUE_BOX_Y             (LAYOUT_CONTENT_Y + LAYOUT_VALUE_BOX_MARGIN_TOP)
#define LAYOUT_VALUE_BOX_HEIGHT        56U
#define LAYOUT_VALUE_BOX_WIDTH         148U
#define LAYOUT_VALUE_BOX_GAP           10U
#define LAYOUT_VALUE_BOX_X1            LAYOUT_GRID_MARGIN
#define LAYOUT_VALUE_BOX_X2            (LAYOUT_VALUE_BOX_X1 + LAYOUT_VALUE_BOX_WIDTH + LAYOUT_VALUE_BOX_GAP)

/* Mode Line (Auto Mode + DateTime) -----------------------------------------*/
#define LAYOUT_MODE_LINE_Y           (LAYOUT_VALUE_BOX_Y + LAYOUT_VALUE_BOX_HEIGHT + 6U)
#define LAYOUT_MODE_LINE_HEIGHT      20U

/* Parcel Progress Line -----------------------------------------------------*/
#define LAYOUT_PARCEL_LINE_Y         (LAYOUT_MODE_LINE_Y + LAYOUT_MODE_LINE_HEIGHT + 4U)
#define LAYOUT_PARCEL_LINE_HEIGHT    20U

/* Progress Bar -------------------------------------------------------------*/
#define LAYOUT_PROGRESSBAR_Y         (LAYOUT_PARCEL_LINE_Y + LAYOUT_PARCEL_LINE_HEIGHT + 6U)
#define LAYOUT_PROGRESSBAR_HEIGHT    20U
#define LAYOUT_PROGRESSBAR_WIDTH     (LAYOUT_SCREEN_WIDTH - (2U * LAYOUT_GRID_MARGIN))
#define LAYOUT_PROGRESSBAR_X         LAYOUT_GRID_MARGIN

/* Action Buttons (Main Screen) - Footer içinde -----------------------------*/
#define LAYOUT_ACTION_BUTTONS_Y      LAYOUT_FOOTER_Y
#define LAYOUT_ACTION_BUTTON_HEIGHT  36U
#define LAYOUT_ACTION_BUTTON_WIDTH   100U
#define LAYOUT_ACTION_BUTTON_GAP     10U

/* Button Sizing (Touch Targets) --------------------------------------------*/
#define LAYOUT_BUTTON_MIN_WIDTH      64U
#define LAYOUT_BUTTON_MIN_HEIGHT     36U
#define LAYOUT_BUTTON_PADDING_X      12U
#define LAYOUT_BUTTON_PADDING_Y      8U

/* Settings Grid (2x4) ------------------------------------------------------*/
#define LAYOUT_SETTINGS_GRID_COLS    2U
#define LAYOUT_SETTINGS_GRID_ROWS    4U
#define LAYOUT_SETTINGS_CELL_WIDTH   ((LAYOUT_SCREEN_WIDTH - LAYOUT_GRID_MARGIN) / LAYOUT_SETTINGS_GRID_COLS)
#define LAYOUT_SETTINGS_CELL_HEIGHT  ((LAYOUT_CONTENT_HEIGHT - LAYOUT_FOOTER_HEIGHT) / LAYOUT_SETTINGS_GRID_ROWS)

/* Spacing Constants --------------------------------------------------------*/
#define LAYOUT_SPACING_XS    4U    /* Extra small spacing */
#define LAYOUT_SPACING_SM    8U    /* Small spacing */
#define LAYOUT_SPACING_MD    12U   /* Medium spacing */
#define LAYOUT_SPACING_LG    16U   /* Large spacing */
#define LAYOUT_SPACING_XL    24U   /* Extra large spacing */

/* Color Scheme -------------------------------------------------------------*/
/* Primary Colors */
#define LAYOUT_COLOR_PRIMARY       ILI9341_BLUE
#define LAYOUT_COLOR_PRIMARY_DARK  ILI9341_NAVY
#define LAYOUT_COLOR_ACCENT        ILI9341_CYAN

/* Status Colors */
#define LAYOUT_COLOR_SUCCESS       ILI9341_GREEN
#define LAYOUT_COLOR_WARNING       ILI9341_YELLOW
#define LAYOUT_COLOR_ERROR         ILI9341_RED
#define LAYOUT_COLOR_INFO          ILI9341_CYAN

/* Neutral Colors */
#define LAYOUT_COLOR_BG            ILI9341_BLACK
#define LAYOUT_COLOR_BG_DARK       ILI9341_NAVY
#define LAYOUT_COLOR_BG_LIGHT      ILI9341_DARKGRAY
#define LAYOUT_COLOR_TEXT          ILI9341_WHITE
#define LAYOUT_COLOR_TEXT_DIM      ILI9341_GRAY

/* Value Box Colors */
#define LAYOUT_COLOR_PH            ILI9341_CYAN
#define LAYOUT_COLOR_EC            ILI9341_GREEN
#define LAYOUT_COLOR_TEMP          ILI9341_ORANGE
#define LAYOUT_COLOR_HUMIDITY      ILI9341_BLUE

/* Typography ---------------------------------------------------------------*/
#define LAYOUT_FONT_SMALL          &Font_8x5
#define LAYOUT_FONT_NORMAL         &Font_16x8
#define LAYOUT_FONT_LARGE          &Font_24x16
#define LAYOUT_FONT_XLARGE         &Font_32x24

/* Value Display Format -----------------------------------------------------*/
#define LAYOUT_VALUE_DECIMAL_PLACES    2U
#define LAYOUT_VALUE_MAX_LENGTH        16U

/* Touch Calibration --------------------------------------------------------*/
#define LAYOUT_CALIB_MARGIN        40U
#define LAYOUT_CALIB_POINT_RADIUS  15U

/* Animation & Timing -------------------------------------------------------*/
#define LAYOUT_TRANSITION_MS       200U
#define LAYOUT_DEBOUNCE_MS         120U
#define LAYOUT_REFRESH_RATE_MS     50U

/* Utility Macros -----------------------------------------------------------*/
#define LAYOUT_CENTER_X(w)         ((LAYOUT_SCREEN_WIDTH - (w)) / 2U)
#define LAYOUT_CENTER_Y(h)         ((LAYOUT_SCREEN_HEIGHT - (h)) / 2U)
#define LAYOUT_RIGHT_X(w)          (LAYOUT_SCREEN_WIDTH - (w) - LAYOUT_GRID_MARGIN)
#define LAYOUT_BOTTOM_Y(h)         (LAYOUT_SCREEN_HEIGHT - (h) - LAYOUT_GRID_MARGIN)

/* Grid Position Calculator -------------------------------------------------*/
#define LAYOUT_GRID_X(col)         (LAYOUT_GRID_MARGIN + ((col) * (LAYOUT_GRID_CELL_W + LAYOUT_GRID_GAP)))
#define LAYOUT_GRID_Y(row)         (LAYOUT_GRID_MARGIN + ((row) * (LAYOUT_GRID_CELL_H + LAYOUT_GRID_GAP)))
#define LAYOUT_GRID_W(cols)        ((cols) * LAYOUT_GRID_CELL_W + ((cols) - 1U) * LAYOUT_GRID_GAP)
#define LAYOUT_GRID_H(rows)        ((rows) * LAYOUT_GRID_CELL_H + ((rows) - 1U) * LAYOUT_GRID_GAP)

#ifdef __cplusplus
}
#endif

#endif /* __GUI_LAYOUT_H */
