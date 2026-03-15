/**
 ******************************************************************************
 * @file           : touch_xpt2046.h
 * @brief          : XPT2046 Touch Screen Controller Driver
 ******************************************************************************
 */

#ifndef __TOUCH_XPT2046_H
#define __TOUCH_XPT2046_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

/* Touch Configuration ------------------------------------------------------*/
#define TOUCH_SPI SPI1
#define TOUCH_CS_PIN GPIO_PIN_10
#define TOUCH_CS_PORT GPIOB
#define TOUCH_IRQ_PIN GPIO_PIN_15
#define TOUCH_IRQ_PORT GPIOC
#define TOUCH_IRQ_EXTI EXTI15_10_IRQn

/* Touch Threshold ----------------------------------------------------------*/
#define TOUCH_THRESHOLD 800U /* Touch detection threshold */

/* Calibration Points -------------------------------------------------------*/
#define TOUCH_CAL_POINTS 5U

/* Touch Commands -----------------------------------------------------------*/
#define XPT2046_START 0x80U     /* Start bit */
#define XPT2046_ADDR_X 0x90U    /* X position */
#define XPT2046_ADDR_Y 0xD0U    /* Y position */
#define XPT2046_ADDR_Z1 0xB0U   /* Z1 position */
#define XPT2046_ADDR_Z2 0xC0U   /* Z2 position */
#define XPT2046_ADDR_BAT 0xA0U  /* Battery */
#define XPT2046_ADDR_TEMP 0x80U /* Temperature */
#define XPT2046_ADDR_AUX 0xE0U  /* Auxiliary */

/* Touch Structure ----------------------------------------------------------*/
typedef struct {
  int16_t x;
  int16_t y;
  uint8_t pressed;
  uint16_t raw_x;
  uint16_t raw_y;
  uint16_t raw_z;
} touch_point_t;

/* Calibration Data Structure -----------------------------------------------*/
typedef struct {
  int32_t a; /* X scale */
  int32_t b; /* Y scale */
  int32_t c; /* X offset */
  int32_t d; /* Y offset */
  int32_t e; /* X divider */
  int32_t f; /* Y divider */
  uint8_t valid;
} touch_calibration_t;

/* Touch Functions ----------------------------------------------------------*/
void TOUCH_Init(void);
uint8_t TOUCH_IsPressed(void);
uint8_t TOUCH_ReadPoint(touch_point_t *point);
void TOUCH_GetPoint(touch_point_t *point);
void TOUCH_CalibrationStart(void);
uint8_t TOUCH_CalibrationProcess(uint16_t *x, uint16_t *y, uint8_t *step);
void TOUCH_SetCalibration(const touch_calibration_t *cal);
void TOUCH_GetCalibration(touch_calibration_t *cal);
void TOUCH_SaveCalibration(void);
void TOUCH_LoadCalibration(void);

/* Callbacks ---------------------------------------------------------------*/
void TOUCH_IRQHandler_Callback(void);

#ifdef __cplusplus
}
#endif

#endif /* __TOUCH_XPT2046_H */