/**
 ******************************************************************************
 * @file           : usb_config.c
 * @brief          : USB tabanli sulama programi import/export protokolu
 ******************************************************************************
 */

#include "main.h"
#include "fault_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USB_CONFIG_PROTOCOL_VERSION 2U
#define USB_CONFIG_FIRMWARE_VERSION "core-0.1"
#define USB_CONFIG_DEVICE_NAME "irrigation-core"

typedef struct {
  char line_buffer[USB_CONFIG_LINE_BUFFER_SIZE];
  uint16_t line_length;
  uint8_t session_active;
  usb_config_status_t status;
  char status_text[48];
} usb_config_context_t;

static usb_config_context_t g_usb_config = {0};

static void USB_CONFIG_SetStatus(usb_config_status_t status, const char *text);
static void USB_CONFIG_ProcessByte(uint8_t value);
static void USB_CONFIG_HandleLine(char *line);
static void USB_CONFIG_SendText(const char *text);
static void USB_CONFIG_SendResponse(const char *prefix,
                                    const irrigation_program_t *program,
                                    uint8_t program_id);
static void USB_CONFIG_SendError(const char *code, const char *detail);
static uint8_t USB_CONFIG_ParseUInt32(const char *token, uint32_t *value);
static uint8_t USB_CONFIG_ParseInt32(const char *token, int32_t *value);
static uint8_t USB_CONFIG_IsHHMMValid(uint32_t hhmm);
static uint8_t USB_CONFIG_ApplyProgramTokens(char **tokens, uint8_t token_count);
static void USB_CONFIG_SendDeviceInfo(void);
static void USB_CONFIG_SendTelemetrySnapshot(void);
static void USB_CONFIG_SendFaultEvent(void);
static void USB_CONFIG_SendRuntimeEvent(void);
static void USB_CONFIG_SendHelp(void);
static void USB_CONFIG_ExportAll(void);

__attribute__((weak)) uint16_t USB_CONFIG_TransportRead(uint8_t *buffer,
                                                        uint16_t max_length) {
  (void)buffer;
  (void)max_length;
  return 0U;
}

__attribute__((weak)) void USB_CONFIG_TransportWrite(const uint8_t *data,
                                                     uint16_t length) {
  (void)data;
  (void)length;
}

void USB_CONFIG_Init(void) {
  memset(&g_usb_config, 0, sizeof(g_usb_config));
  USB_CONFIG_SetStatus(USB_CONFIG_STATUS_IDLE, "USB config idle");
}

void USB_CONFIG_Process(void) {
  uint8_t rx_buffer[USB_CONFIG_RX_BUFFER_SIZE] = {0};
  uint16_t received = USB_CONFIG_TransportRead(rx_buffer, sizeof(rx_buffer));

  if (received == 0U) {
    return;
  }

  USB_CONFIG_FeedRx(rx_buffer, received);
}

void USB_CONFIG_FeedRx(const uint8_t *data, uint16_t length) {
  uint16_t i = 0U;

  if (data == NULL || length == 0U) {
    return;
  }

  g_usb_config.session_active = 1U;
  LOW_POWER_UpdateActivity();
  if (g_usb_config.status == USB_CONFIG_STATUS_IDLE) {
    USB_CONFIG_SetStatus(USB_CONFIG_STATUS_CONNECTED, "USB session active");
  }

  for (i = 0U; i < length; i++) {
    USB_CONFIG_ProcessByte(data[i]);
  }
}

usb_config_status_t USB_CONFIG_GetStatus(void) { return g_usb_config.status; }

const char *USB_CONFIG_GetStatusText(void) { return g_usb_config.status_text; }

uint8_t USB_CONFIG_IsSessionActive(void) { return g_usb_config.session_active; }

static void USB_CONFIG_SetStatus(usb_config_status_t status, const char *text) {
  g_usb_config.status = status;
  snprintf(g_usb_config.status_text, sizeof(g_usb_config.status_text), "%s",
           (text != NULL) ? text : "");
}

static void USB_CONFIG_ProcessByte(uint8_t value) {
  if (value == '\r') {
    return;
  }

  if (value == '\n') {
    g_usb_config.line_buffer[g_usb_config.line_length] = '\0';
    if (g_usb_config.line_length != 0U) {
      USB_CONFIG_HandleLine(g_usb_config.line_buffer);
    }
    g_usb_config.line_length = 0U;
    g_usb_config.line_buffer[0] = '\0';
    return;
  }

  if (g_usb_config.line_length >= (USB_CONFIG_LINE_BUFFER_SIZE - 1U)) {
    g_usb_config.line_length = 0U;
    g_usb_config.line_buffer[0] = '\0';
    USB_CONFIG_SetStatus(USB_CONFIG_STATUS_RX_OVERFLOW,
                         "USB line buffer overflow");
    USB_CONFIG_SendError("OVERFLOW", "Line too long");
    return;
  }

  g_usb_config.line_buffer[g_usb_config.line_length++] = (char)value;
}

static void USB_CONFIG_HandleLine(char *line) {
  char *tokens[21] = {0};
  char *token = NULL;
  uint8_t token_count = 0U;

  token = strtok(line, ",");
  while (token != NULL && token_count < (uint8_t)(sizeof(tokens) / sizeof(tokens[0]))) {
    tokens[token_count++] = token;
    token = strtok(NULL, ",");
  }

  if (token_count == 0U) {
    return;
  }

  if (strcmp(tokens[0], "PING") == 0) {
    USB_CONFIG_SetStatus(USB_CONFIG_STATUS_CONNECTED, "USB ping ok");
    USB_CONFIG_SendText("OK,PONG\r\n");
    return;
  }

  if (strcmp(tokens[0], "HELP") == 0) {
    USB_CONFIG_SendHelp();
    return;
  }

  if (strcmp(tokens[0], "LIST") == 0) {
    USB_CONFIG_ExportAll();
    return;
  }

  if (strcmp(tokens[0], "INFO") == 0) {
    USB_CONFIG_SendDeviceInfo();
    return;
  }

  if (strcmp(tokens[0], "TELEM") == 0) {
    USB_CONFIG_SendTelemetrySnapshot();
    return;
  }

  if (strcmp(tokens[0], "FAULT") == 0) {
    USB_CONFIG_SendFaultEvent();
    return;
  }

  if (strcmp(tokens[0], "RUNTIME") == 0) {
    USB_CONFIG_SendRuntimeEvent();
    return;
  }

  if (strcmp(tokens[0], "GET") == 0) {
    irrigation_program_t program = {0};
    uint32_t program_id = 0U;

    if (token_count != 2U || !USB_CONFIG_ParseUInt32(tokens[1], &program_id) ||
        program_id == 0U || program_id > IRRIGATION_PROGRAM_COUNT) {
      USB_CONFIG_SetStatus(USB_CONFIG_STATUS_PARSE_ERROR, "GET parse error");
      USB_CONFIG_SendError("BAD_GET", "Use GET,<id>");
      return;
    }

    IRRIGATION_CTRL_GetProgram((uint8_t)program_id, &program);
    USB_CONFIG_SendResponse("OK,PROGRAM", &program, (uint8_t)program_id);
    return;
  }

  if (strcmp(tokens[0], "SET") == 0) {
    if (USB_CONFIG_ApplyProgramTokens(tokens, token_count) == 0U) {
      USB_CONFIG_SetStatus(USB_CONFIG_STATUS_PARSE_ERROR, "SET parse error");
      return;
    }
    return;
  }

  USB_CONFIG_SetStatus(USB_CONFIG_STATUS_PARSE_ERROR, "Unknown USB command");
  USB_CONFIG_SendError("UNKNOWN", "Use HELP");
}

static void USB_CONFIG_SendText(const char *text) {
  size_t length = 0U;

  if (text == NULL) {
    return;
  }

  length = strlen(text);
  if (length == 0U) {
    return;
  }

  USB_CONFIG_TransportWrite((const uint8_t *)text, (uint16_t)length);
}

static void USB_CONFIG_SendResponse(const char *prefix,
                                    const irrigation_program_t *program,
                                    uint8_t program_id) {
  char response[USB_CONFIG_MAX_RESPONSE_SIZE] = {0};

  if (prefix == NULL || program == NULL) {
    return;
  }

  snprintf(response, sizeof(response),
           "%s,%u,%u,%04u,%04u,%u,%u,%u,%u,%u,%d,%d,%u,%u,%u,%u,%u,%u\r\n",
           prefix, (unsigned int)program_id, (unsigned int)program->enabled,
           (unsigned int)program->start_hhmm, (unsigned int)program->end_hhmm,
           (unsigned int)program->valve_mask,
           (unsigned int)program->irrigation_min,
           (unsigned int)program->wait_min,
           (unsigned int)program->repeat_count,
           (unsigned int)program->days_mask, (int)program->ph_set_x100,
           (int)program->ec_set_x100,
           (unsigned int)program->fert_ratio_percent[0],
           (unsigned int)program->fert_ratio_percent[1],
           (unsigned int)program->fert_ratio_percent[2],
           (unsigned int)program->fert_ratio_percent[3],
           (unsigned int)program->pre_flush_sec,
           (unsigned int)program->post_flush_sec);
  USB_CONFIG_SendText(response);
}

static void USB_CONFIG_SendError(const char *code, const char *detail) {
  char response[USB_CONFIG_MAX_RESPONSE_SIZE] = {0};

  snprintf(response, sizeof(response), "ERR,%s,%s\r\n",
           (code != NULL) ? code : "UNKNOWN",
           (detail != NULL) ? detail : "");
  USB_CONFIG_SendText(response);
}

static uint8_t USB_CONFIG_ParseUInt32(const char *token, uint32_t *value) {
  char *end_ptr = NULL;
  unsigned long parsed = 0UL;

  if (token == NULL || value == NULL || *token == '\0') {
    return 0U;
  }

  parsed = strtoul(token, &end_ptr, 10);
  if (end_ptr == NULL || *end_ptr != '\0') {
    return 0U;
  }

  *value = (uint32_t)parsed;
  return 1U;
}

static uint8_t USB_CONFIG_ParseInt32(const char *token, int32_t *value) {
  char *end_ptr = NULL;
  long parsed = 0L;

  if (token == NULL || value == NULL || *token == '\0') {
    return 0U;
  }

  parsed = strtol(token, &end_ptr, 10);
  if (end_ptr == NULL || *end_ptr != '\0') {
    return 0U;
  }

  *value = (int32_t)parsed;
  return 1U;
}

static uint8_t USB_CONFIG_IsHHMMValid(uint32_t hhmm) {
  uint32_t hours = hhmm / 100U;
  uint32_t minutes = hhmm % 100U;

  if (hours > 23U || minutes > 59U) {
    return 0U;
  }

  return 1U;
}

static uint8_t USB_CONFIG_ApplyProgramTokens(char **tokens, uint8_t token_count) {
  irrigation_program_t program = {0};
  uint32_t values[18] = {0};
  int32_t ph_value = 0;
  int32_t ec_value = 0;
  uint8_t i = 0U;
  uint8_t program_id = 0U;

  if (tokens == NULL ||
      (token_count != 12U && token_count != 16U && token_count != 18U &&
       token_count != 20U &&
       token_count != 21U)) {
    USB_CONFIG_SendError(
        "BAD_SET",
        "Use SET,id,enabled,start,end,mask,irr,wait,repeat,days,ph,ec");
    return 0U;
  }

  for (i = 1U; i <= 9U; i++) {
    if (!USB_CONFIG_ParseUInt32(tokens[i], &values[i - 1U])) {
      USB_CONFIG_SendError("BAD_NUMBER", "Invalid unsigned field");
      return 0U;
    }
  }

  if (!USB_CONFIG_ParseInt32(tokens[10], &ph_value) ||
      !USB_CONFIG_ParseInt32(tokens[11], &ec_value)) {
    USB_CONFIG_SendError("BAD_NUMBER", "Invalid signed field");
    return 0U;
  }

  program_id = (uint8_t)values[0];
  if (program_id == 0U || program_id > IRRIGATION_PROGRAM_COUNT) {
    USB_CONFIG_SendError("BAD_ID", "Program id out of range");
    return 0U;
  }

  if (values[1] > 1U) {
    USB_CONFIG_SendError("BAD_ENABLED", "Enabled must be 0 or 1");
    return 0U;
  }

  if (!USB_CONFIG_IsHHMMValid(values[2]) || !USB_CONFIG_IsHHMMValid(values[3])) {
    USB_CONFIG_SendError("BAD_TIME", "HHMM must be 0000..2359");
    return 0U;
  }

  if (values[4] == 0U ||
      (values[4] & ~((1UL << IRRIGATION_PROGRAM_VALVE_COUNT) - 1UL)) != 0UL) {
    USB_CONFIG_SendError("BAD_MASK", "Valve mask is invalid");
    return 0U;
  }

  if (values[5] == 0U || values[5] > 999U) {
    USB_CONFIG_SendError("BAD_IRR", "Irrigation minutes must be 1..999");
    return 0U;
  }

  if (values[6] > 999U) {
    USB_CONFIG_SendError("BAD_WAIT", "Wait minutes must be 0..999");
    return 0U;
  }

  if (values[7] == 0U || values[7] > 99U) {
    USB_CONFIG_SendError("BAD_REPEAT", "Repeat count must be 1..99");
    return 0U;
  }

  if (values[8] == 0U || values[8] > 0x7FU) {
    USB_CONFIG_SendError("BAD_DAYS", "Days mask must be 1..127");
    return 0U;
  }

  if (ph_value < 0 || ph_value > 1400) {
    USB_CONFIG_SendError("BAD_PH", "PH x100 must be 0..1400");
    return 0U;
  }

  if (ec_value < 0 || ec_value > 2000) {
    USB_CONFIG_SendError("BAD_EC", "EC x100 must be 0..2000");
    return 0U;
  }

  IRRIGATION_CTRL_GetProgram(program_id, &program);
  program.enabled = (uint8_t)values[1];
  program.start_hhmm = (uint16_t)values[2];
  program.end_hhmm = (uint16_t)values[3];
  program.valve_mask = (uint8_t)values[4];
  program.irrigation_min = (uint16_t)values[5];
  program.wait_min = (uint16_t)values[6];
  program.repeat_count = (uint8_t)values[7];
  program.days_mask = (uint8_t)values[8];
  program.ph_set_x100 = (int16_t)ph_value;
  program.ec_set_x100 = (int16_t)ec_value;
  if (token_count >= 16U) {
    for (i = 12U; i <= 15U; i++) {
      uint32_t ratio = 0U;
      if (!USB_CONFIG_ParseUInt32(tokens[i], &ratio) || ratio > 100U) {
        USB_CONFIG_SendError("BAD_RATIO", "Fertilizer ratios must be 0..100");
        return 0U;
      }
      program.fert_ratio_percent[i - 12U] = (uint8_t)ratio;
    }
  }

  if (token_count == 18U) {
    uint32_t pre_flush_sec = 0U;
    uint32_t post_flush_sec = 0U;

    if (!USB_CONFIG_ParseUInt32(tokens[16], &pre_flush_sec) ||
        !USB_CONFIG_ParseUInt32(tokens[17], &post_flush_sec)) {
      USB_CONFIG_SendError("BAD_FLUSH", "Flush seconds must be unsigned");
      return 0U;
    }

    if (pre_flush_sec > IRRIGATION_MAX_FLUSH_SEC ||
        post_flush_sec > IRRIGATION_MAX_FLUSH_SEC) {
      USB_CONFIG_SendError("BAD_FLUSH", "Flush seconds must be 0..900");
      return 0U;
    }

    program.pre_flush_sec = (uint16_t)pre_flush_sec;
    program.post_flush_sec = (uint16_t)post_flush_sec;
  }

  program.last_run_day = 0U;
  program.last_run_minute = 0xFFFFU;
  IRRIGATION_CTRL_SetProgram(program_id, &program);

  /* Respond first so the desktop tool does not appear to hang while EEPROM
   * persistence runs on the maintenance path. */
  USB_CONFIG_SetStatus(USB_CONFIG_STATUS_APPLIED, "Program applied from USB");
  USB_CONFIG_SendResponse("OK,SET", &program, program_id);
  IRRIGATION_CTRL_MaintenanceTask();
  return 1U;
}

static void USB_CONFIG_SendDeviceInfo(void) {
  char response[USB_CONFIG_MAX_RESPONSE_SIZE] = {0};

  (void)snprintf(response, sizeof(response),
                 "OK,DEVICE,%s,%s,%u,%s,%u,%u,%u,%lu\r\n",
                 USB_CONFIG_DEVICE_NAME, USB_CONFIG_FIRMWARE_VERSION,
                 (unsigned int)USB_CONFIG_PROTOCOL_VERSION, "STM32F407VETx",
                 (unsigned int)IRRIGATION_PROGRAM_COUNT,
                 (unsigned int)PARCEL_VALVE_COUNT,
                 (unsigned int)DOSING_VALVE_COUNT,
                 (unsigned long)HAL_GetTick());
  USB_CONFIG_SendText(response);
}

static void USB_CONFIG_SendTelemetrySnapshot(void) {
  char response[USB_CONFIG_MAX_RESPONSE_SIZE] = {0};
  int16_t ph_x100 = (int16_t)(IRRIGATION_CTRL_GetPH() * 100.0f);
  int16_t ec_x100 = (int16_t)(IRRIGATION_CTRL_GetEC() * 100.0f);
  fault_manager_status_t fault = {0};

  FAULT_MGR_GetStatus(&fault);
  (void)snprintf(response, sizeof(response),
                 "OK,TELEMETRY,%lu,%u,%u,%u,%u,%u,%lu,%d,%d,%u,%u,%u,%u,%u\r\n",
                 (unsigned long)HAL_GetTick(),
                 (unsigned int)IRRIGATION_CTRL_GetState(),
                 (unsigned int)IRRIGATION_CTRL_GetProgramState(),
                 (unsigned int)IRRIGATION_CTRL_IsRunning(),
                 (unsigned int)IRRIGATION_CTRL_GetActiveProgram(),
                 (unsigned int)IRRIGATION_CTRL_GetCurrentParcelId(),
                 (unsigned long)IRRIGATION_CTRL_GetRemainingTime(),
                 (int)ph_x100, (int)ec_x100,
                 (unsigned int)PH_GetStatus(),
                 (unsigned int)EC_GetStatus(),
                 (unsigned int)VALVES_GetActiveMask(),
                 (unsigned int)IRRIGATION_CTRL_GetLastError(),
                 (unsigned int)fault.active);
  USB_CONFIG_SendText(response);
}

static void USB_CONFIG_SendFaultEvent(void) {
  char response[USB_CONFIG_MAX_RESPONSE_SIZE] = {0};
  fault_manager_status_t fault = {0};

  FAULT_MGR_GetStatus(&fault);
  (void)snprintf(response, sizeof(response),
                 "OK,FAULT,%u,%u,%u,%u,%u,%u,%u,%s\r\n",
                 (unsigned int)fault.active,
                 (unsigned int)fault.latched,
                 (unsigned int)fault.manual_ack_required,
                 (unsigned int)fault.acknowledged,
                 (unsigned int)fault.last_error,
                 (unsigned int)fault.valve_error_mask,
                 (unsigned int)fault.recommended_state,
                 fault.alarm_text);
  USB_CONFIG_SendText(response);
}

static void USB_CONFIG_SendRuntimeEvent(void) {
  char response[USB_CONFIG_MAX_RESPONSE_SIZE] = {0};
  irrigation_runtime_backup_t backup = {0};

  IRRIGATION_CTRL_GetRuntimeBackup(&backup);
  (void)snprintf(response, sizeof(response),
                 "OK,RUNTIME,%u,%u,%u,%u,%u,%u,%u,%d,%d,%u\r\n",
                 (unsigned int)backup.valid,
                 (unsigned int)backup.active_program_id,
                 (unsigned int)backup.active_valve_index,
                 (unsigned int)backup.repeat_index,
                 (unsigned int)backup.program_state,
                 (unsigned int)backup.active_valve_id,
                 (unsigned int)backup.remaining_sec,
                 (int)backup.ec_target_x100,
                 (int)backup.ph_target_x100,
                 (unsigned int)backup.error_code);
  USB_CONFIG_SendText(response);
}

static void USB_CONFIG_SendHelp(void) {
  USB_CONFIG_SendText("OK,HELP,PING\r\n");
  USB_CONFIG_SendText("OK,HELP,INFO\r\n");
  USB_CONFIG_SendText("OK,HELP,TELEM\r\n");
  USB_CONFIG_SendText("OK,HELP,FAULT\r\n");
  USB_CONFIG_SendText("OK,HELP,RUNTIME\r\n");
  USB_CONFIG_SendText("OK,HELP,LIST\r\n");
  USB_CONFIG_SendText("OK,HELP,GET,<id>\r\n");
  USB_CONFIG_SendText(
      "OK,HELP,SET,<id>,<enabled>,<start>,<end>,<mask>,<irr>,<wait>,<repeat>,"
      "<days>,<ph_x100>,<ec_x100>,<fert_a_pct>,<fert_b_pct>,<fert_c_pct>,"
      "<fert_d_pct>,<pre_flush_sec>,<post_flush_sec>\r\n");
}

static void USB_CONFIG_ExportAll(void) {
  irrigation_program_t program = {0};
  uint8_t i = 0U;

  USB_CONFIG_SendText("OK,LIST,BEGIN\r\n");
  for (i = 0U; i < IRRIGATION_PROGRAM_COUNT; i++) {
    IRRIGATION_CTRL_GetProgram(i + 1U, &program);
    USB_CONFIG_SendResponse("PROGRAM", &program, i + 1U);
  }
  USB_CONFIG_SendText("OK,LIST,END\r\n");
}
