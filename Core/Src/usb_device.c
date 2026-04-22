/**
 ******************************************************************************
 * @file           : usb_device.c
 * @brief          : USB device stack bootstrap
 ******************************************************************************
 */

#include "main.h"
#include "usb_device.h"
#include "usbd_cdc.h"
#include "usbd_cdc_interface.h"
#include "usbd_desc.h"

USBD_HandleTypeDef hUsbDeviceFS;

void MX_USB_DEVICE_Init(void) {
  if (USBD_Init(&hUsbDeviceFS, &VCP_Desc, 0U) != USBD_OK) {
    Error_Handler();
  }

  if (USBD_RegisterClass(&hUsbDeviceFS, USBD_CDC_CLASS) != USBD_OK) {
    Error_Handler();
  }

  if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &g_usbd_cdc_interface_fops) !=
      USBD_OK) {
    Error_Handler();
  }

  if (USBD_Start(&hUsbDeviceFS) != USBD_OK) {
    Error_Handler();
  }
}
