/**
 ******************************************************************************
 * @file           : usbd_cdc_interface.h
 * @brief          : USB CDC application bridge
 ******************************************************************************
 */

#ifndef __USBD_CDC_INTERFACE_H
#define __USBD_CDC_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_cdc.h"

extern USBD_CDC_ItfTypeDef g_usbd_cdc_interface_fops;

uint8_t CDC_Transmit_FS(uint8_t *buffer, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif /* __USBD_CDC_INTERFACE_H */
