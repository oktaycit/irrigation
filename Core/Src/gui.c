/**
 ******************************************************************************
 * @file           : gui.c
 * @brief          : Grafiksel Kullanıcı Arayüzü (GUI) Driver Implementation
 ******************************************************************************
 */

#include "main.h"
#include <stdio.h>
#include <string.h>

/* Internal Handle */
static gui_handle_t h_gui = {0};

static void GUI_DrawSplashScreen(void);

/**
 * @brief  Initialize GUI
 */
void GUI_Init(void) {
    memset(&h_gui, 0, sizeof(gui_handle_t));
    
    /* Set LCD to landscape for better menu display */
    LCD_SetOrientation(LCD_ORIENTATION_LANDSCAPE);
    LCD_Clear(ILI9341_BLACK);
    
    h_gui.is_initialized = 1;
    h_gui.current_screen = SCREEN_SPLASH;
    
    /* Draw Splash Screen */
    GUI_DrawScreen(SCREEN_SPLASH);
    HAL_Delay(1000);
    
    GUI_NavigateTo(SCREEN_MAIN);
}

/**
 * @brief  GUI Periodic Update
 */
void GUI_Update(void) {
    if (!h_gui.is_initialized) return;

    /* Check for touch */
    touch_point_t p;
    if (TOUCH_ReadPoint(&p)) {
        GUI_ProcessTouch(&p);
    }

    /* Periodic display updates (values, status bars, etc.) */
    static uint32_t last_update = 0;
    if (HAL_GetTick() - last_update > 250) {
        last_update = HAL_GetTick();
        if (h_gui.current_screen == SCREEN_MAIN) {
            GUI_DrawMainScreen();
        }
    }
}

/**
 * @brief  Navigate to a specific screen
 */
void GUI_NavigateTo(screen_id_t screen_id) {
    if (screen_id >= SCREEN_MAX) return;
    
    h_gui.current_screen = screen_id;
    LCD_Clear(ILI9341_BLACK);
    GUI_DrawScreen(screen_id);
}

/**
 * @brief  Draw Main Screen Layout
 */
void GUI_DrawMainScreen(void) {
    /* Header (pH and EC) */
    char buf[32];
    sprintf(buf, "pH: %.1f", PH_GetValue());
    LCD_DrawString(10, 10, buf, ILI9341_CYAN, ILI9341_BLACK, &Font_16x8);

    sprintf(buf, "EC: %.2f mS/cm", EC_GetValue());
    LCD_DrawString(140, 10, buf, ILI9341_GREEN, ILI9341_BLACK, &Font_16x8);

    /* Separator */
    LCD_DrawHLine(0, 35, 320, ILI9341_GRAY);

    /* Irrigation Status */
    if (IRRIGATION_CTRL_IsRunning()) {
        uint8_t parcel_id = IRRIGATION_CTRL_GetCurrentParcelId();
        sprintf(buf, "Parsel %d - Sulaniyor", parcel_id);
        LCD_DrawString(10, 60, buf, ILI9341_WHITE, ILI9341_BLACK, &Font_16x8);

        /* Progress Bar */
        uint32_t rem = IRRIGATION_CTRL_GetRemainingTime();
        sprintf(buf, "%02d:%02d kaldi", (int)(rem / 60), (int)(rem % 60));
        LCD_DrawString(10, 120, buf, ILI9341_YELLOW, ILI9341_BLACK, &Font_16x8);
        
        /* Simple Progress Bar Draw */
        LCD_DrawRect(10, 90, 300, 20, ILI9341_GRAY);
        LCD_FillRect(11, 91, 200, 18, ILI9341_BLUE); /* 66% placeholder */
    } else {
        LCD_DrawString(60, 90, "SISTEM BEKLEMEDE", ILI9341_GRAY, ILI9341_BLACK, &Font_24x16);
    }

    /* Buttons (Bottom) */
    GUI_DrawButton(10, 190, 90, 40, "MENU", ILI9341_DARKGRAY);
    GUI_DrawButton(115, 190, 90, 40, "MANUEL", ILI9341_DARKGRAY);
    GUI_DrawButton(220, 190, 90, 40, "AYARLAR", ILI9341_DARKGRAY);
}

/**
 * @brief  Draw a simple button
 */
void GUI_DrawButton(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char *text, lcd_color_t color) {
    LCD_FillRect(x, y, w, h, color);
    LCD_DrawRect(x, y, w, h, ILI9341_WHITE);
    
    /* Center text */
    uint16_t tw = strlen(text) * 8;
    LCD_DrawString(x + (w - tw) / 2, y + (h - 16) / 2, text, ILI9341_WHITE, color, &Font_16x8);
}

/**
 * @brief  Process touch events
 */
void GUI_ProcessTouch(touch_point_t *point) {
    /* Translate coordinates if needed (already handled by driver/calibration) */
    
    /* Check bottom buttons on main screen */
    if (h_gui.current_screen == SCREEN_MAIN) {
        if (point->y > 190) {
            if (point->x < 100) GUI_NavigateTo(SCREEN_SETTINGS);
            else if (point->x < 210) GUI_NavigateTo(SCREEN_MANUAL);
            else GUI_NavigateTo(SCREEN_SETTINGS);
        }
    }
}

/**
 * @brief  Draw common header
 */
void GUI_DrawHeader(const char *title) {
    LCD_FillRect(0, 0, 320, 30, ILI9341_NAVY);
    LCD_DrawString(10, 7, title, ILI9341_WHITE, ILI9341_NAVY, &Font_16x8);
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
            GUI_DrawHeader("Manuel");
            break;
        case SCREEN_SETTINGS:
            GUI_DrawHeader("Ayarlar");
            break;
        default:
            GUI_DrawHeader("Menü");
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

static void GUI_DrawSplashScreen(void) {
    LCD_Clear(ILI9341_BLACK);
    LCD_DrawString(48, 72, "STM32 SULAMA", ILI9341_GREEN, ILI9341_BLACK,
                   &Font_24x16);
    LCD_DrawString(56, 116, "Kontrol Sistemi", ILI9341_CYAN, ILI9341_BLACK,
                   &Font_16x8);
}
