/**
 ******************************************************************************
 * @file           : usbd_cdc_interface.c
 * @brief          : USB CDC bridge for irrigation program configuration
 ******************************************************************************
 */

#include "main.h"
#include "usbd_cdc_interface.h"

#define USB_CDC_RX_PACKET_SIZE CDC_DATA_FS_OUT_PACKET_SIZE
#define USB_CDC_RX_RING_SIZE 512U
#define USB_CDC_TX_TIMEOUT_MS 500U

typedef struct {
  uint8_t buffer[USB_CDC_RX_RING_SIZE];
  volatile uint16_t head;
  volatile uint16_t tail;
} usb_cdc_rx_ring_t;

extern USBD_HandleTypeDef hUsbDeviceFS;

static usb_cdc_rx_ring_t g_usb_cdc_rx_ring = {0};
static uint8_t g_usb_cdc_rx_packet[USB_CDC_RX_PACKET_SIZE] = {0};
static uint8_t g_usb_cdc_tx_busy = 0U;
static USBD_CDC_LineCodingTypeDef g_usb_cdc_line_coding = {
    115200U,
    0x00U,
    0x00U,
    0x08U,
};

static int8_t CDC_Itf_Init(void);
static int8_t CDC_Itf_DeInit(void);
static int8_t CDC_Itf_Control(uint8_t cmd, uint8_t *pbuf, uint16_t length);
static int8_t CDC_Itf_Receive(uint8_t *pbuf, uint32_t *Len);
static int8_t CDC_Itf_TransmitCplt(uint8_t *pbuf, uint32_t *Len, uint8_t epnum);
static void USB_CDC_RingPushByte(uint8_t value);

USBD_CDC_ItfTypeDef g_usbd_cdc_interface_fops = {
    CDC_Itf_Init, CDC_Itf_DeInit, CDC_Itf_Control,
    CDC_Itf_Receive, CDC_Itf_TransmitCplt};

static int8_t CDC_Itf_Init(void) {
  g_usb_cdc_rx_ring.head = 0U;
  g_usb_cdc_rx_ring.tail = 0U;
  g_usb_cdc_tx_busy = 0U;
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, NULL, 0U);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, g_usb_cdc_rx_packet);
  return (int8_t)USBD_CDC_ReceivePacket(&hUsbDeviceFS);
}

static int8_t CDC_Itf_DeInit(void) {
  g_usb_cdc_rx_ring.head = 0U;
  g_usb_cdc_rx_ring.tail = 0U;
  g_usb_cdc_tx_busy = 0U;
  return (int8_t)USBD_OK;
}

static int8_t CDC_Itf_Control(uint8_t cmd, uint8_t *pbuf, uint16_t length) {
  (void)length;

  switch (cmd) {
    case CDC_SET_LINE_CODING:
      g_usb_cdc_line_coding.bitrate =
          (uint32_t)(pbuf[0] | (pbuf[1] << 8) | (pbuf[2] << 16) |
                     (pbuf[3] << 24));
      g_usb_cdc_line_coding.format = pbuf[4];
      g_usb_cdc_line_coding.paritytype = pbuf[5];
      g_usb_cdc_line_coding.datatype = pbuf[6];
      break;

    case CDC_GET_LINE_CODING:
      pbuf[0] = (uint8_t)(g_usb_cdc_line_coding.bitrate);
      pbuf[1] = (uint8_t)(g_usb_cdc_line_coding.bitrate >> 8);
      pbuf[2] = (uint8_t)(g_usb_cdc_line_coding.bitrate >> 16);
      pbuf[3] = (uint8_t)(g_usb_cdc_line_coding.bitrate >> 24);
      pbuf[4] = g_usb_cdc_line_coding.format;
      pbuf[5] = g_usb_cdc_line_coding.paritytype;
      pbuf[6] = g_usb_cdc_line_coding.datatype;
      break;

    default:
      break;
  }

  return (int8_t)USBD_OK;
}

static int8_t CDC_Itf_Receive(uint8_t *pbuf, uint32_t *Len) {
  uint32_t i = 0U;

  for (i = 0U; i < *Len; i++) {
    USB_CDC_RingPushByte(pbuf[i]);
  }

  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, g_usb_cdc_rx_packet);
  return (int8_t)USBD_CDC_ReceivePacket(&hUsbDeviceFS);
}

static int8_t CDC_Itf_TransmitCplt(uint8_t *pbuf, uint32_t *Len, uint8_t epnum) {
  (void)pbuf;
  (void)Len;
  (void)epnum;
  g_usb_cdc_tx_busy = 0U;
  return (int8_t)USBD_OK;
}

uint8_t CDC_Transmit_FS(uint8_t *buffer, uint16_t length) {
  USBD_CDC_HandleTypeDef *cdc_handle = NULL;
  uint32_t start_tick = HAL_GetTick();

  if (buffer == NULL || length == 0U) {
    return (uint8_t)USBD_FAIL;
  }

  while ((HAL_GetTick() - start_tick) < USB_CDC_TX_TIMEOUT_MS) {
    cdc_handle = (USBD_CDC_HandleTypeDef *)hUsbDeviceFS.pClassData;
    if (cdc_handle != NULL && cdc_handle->TxState == 0U &&
        g_usb_cdc_tx_busy == 0U) {
      g_usb_cdc_tx_busy = 1U;
      USBD_CDC_SetTxBuffer(&hUsbDeviceFS, buffer, length);
      if (USBD_CDC_TransmitPacket(&hUsbDeviceFS) == USBD_OK) {
        return (uint8_t)USBD_OK;
      }
      g_usb_cdc_tx_busy = 0U;
    }
  }

  return (uint8_t)USBD_BUSY;
}

uint16_t USB_CONFIG_TransportRead(uint8_t *buffer, uint16_t max_length) {
  uint16_t count = 0U;

  if (buffer == NULL || max_length == 0U) {
    return 0U;
  }

  __disable_irq();
  while (count < max_length && g_usb_cdc_rx_ring.tail != g_usb_cdc_rx_ring.head) {
    buffer[count++] = g_usb_cdc_rx_ring.buffer[g_usb_cdc_rx_ring.tail];
    g_usb_cdc_rx_ring.tail =
        (uint16_t)((g_usb_cdc_rx_ring.tail + 1U) % USB_CDC_RX_RING_SIZE);
  }
  __enable_irq();

  return count;
}

void USB_CONFIG_TransportWrite(const uint8_t *data, uint16_t length) {
  uint16_t offset = 0U;

  if (data == NULL || length == 0U) {
    return;
  }

  while (offset < length) {
    uint16_t chunk_length = length - offset;

    if (chunk_length > CDC_DATA_FS_IN_PACKET_SIZE) {
      chunk_length = CDC_DATA_FS_IN_PACKET_SIZE;
    }

    while (CDC_Transmit_FS((uint8_t *)&data[offset], chunk_length) !=
           (uint8_t)USBD_OK) {
      HAL_Delay(1U);
    }

    while (g_usb_cdc_tx_busy != 0U) {
      HAL_Delay(1U);
    }

    offset = (uint16_t)(offset + chunk_length);
  }
}

static void USB_CDC_RingPushByte(uint8_t value) {
  uint16_t next_head =
      (uint16_t)((g_usb_cdc_rx_ring.head + 1U) % USB_CDC_RX_RING_SIZE);

  if (next_head == g_usb_cdc_rx_ring.tail) {
    return;
  }

  g_usb_cdc_rx_ring.buffer[g_usb_cdc_rx_ring.head] = value;
  g_usb_cdc_rx_ring.head = next_head;
}
