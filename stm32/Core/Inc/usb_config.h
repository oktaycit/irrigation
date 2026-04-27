/**
 ******************************************************************************
 * @file           : usb_config.h
 * @brief          : USB tabanli program import/export arayuzu
 ******************************************************************************
 */

#ifndef __USB_CONFIG_H
#define __USB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Configuration -------------------------------------------------------------*/
#define USB_CONFIG_RX_BUFFER_SIZE 256U
#define USB_CONFIG_LINE_BUFFER_SIZE 160U
#define USB_CONFIG_MAX_RESPONSE_SIZE 192U

typedef enum {
  USB_CONFIG_STATUS_IDLE = 0,
  USB_CONFIG_STATUS_CONNECTED,
  USB_CONFIG_STATUS_RX_OVERFLOW,
  USB_CONFIG_STATUS_PARSE_ERROR,
  USB_CONFIG_STATUS_APPLIED
} usb_config_status_t;

void USB_CONFIG_Init(void);
void USB_CONFIG_Process(void);
void USB_CONFIG_FeedRx(const uint8_t *data, uint16_t length);
usb_config_status_t USB_CONFIG_GetStatus(void);
const char *USB_CONFIG_GetStatusText(void);
uint8_t USB_CONFIG_IsSessionActive(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_CONFIG_H */
