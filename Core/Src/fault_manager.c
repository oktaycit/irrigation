/**
 ******************************************************************************
 * @file           : fault_manager.c
 * @brief          : Merkezilestirilmis hata ve alarm yonetimi
 ******************************************************************************
 */

#include "fault_manager.h"
#include "rtc_driver.h"
#include "valves.h"
#include <stdio.h>
#include <string.h>

static fault_manager_status_t g_fault = {0};

static void FAULT_MGR_CopyValveLabel(uint8_t valve_id, char *buffer,
                                     size_t buffer_size);
static uint8_t FAULT_MGR_FindFirstFaultedValve(void);
static void FAULT_MGR_UpdateUserTexts(void);

static void FAULT_MGR_SetAlarmFromError(error_code_t error) {
  (void)snprintf(g_fault.alarm_text, sizeof(g_fault.alarm_text), "%s",
                 FAULT_MGR_GetErrorString(error));
  FAULT_MGR_UpdateUserTexts();
}

static void FAULT_MGR_CopyValveLabel(uint8_t valve_id, char *buffer,
                                     size_t buffer_size) {
  const char *parcel_name;

  if (buffer == NULL || buffer_size == 0U) {
    return;
  }

  if (valve_id == 0U) {
    (void)snprintf(buffer, buffer_size, "vana");
    return;
  }

  if (valve_id <= PARCEL_VALVE_COUNT) {
    parcel_name = PARCELS_GetName(valve_id);
    if (parcel_name != NULL && parcel_name[0] != '\0') {
      (void)snprintf(buffer, buffer_size, "%s", parcel_name);
    } else {
      (void)snprintf(buffer, buffer_size, "Parsel %u",
                     (unsigned int)valve_id);
    }
    return;
  }

  switch (valve_id) {
  case DOSING_VALVE_ACID_ID:
    (void)snprintf(buffer, buffer_size, "Asit vanasi");
    break;
  case DOSING_VALVE_FERT_A_ID:
    (void)snprintf(buffer, buffer_size, "Gubre A vanasi");
    break;
  case DOSING_VALVE_FERT_B_ID:
    (void)snprintf(buffer, buffer_size, "Gubre B vanasi");
    break;
  case DOSING_VALVE_FERT_C_ID:
    (void)snprintf(buffer, buffer_size, "Gubre C vanasi");
    break;
  case DOSING_VALVE_FERT_D_ID:
    (void)snprintf(buffer, buffer_size, "Gubre D vanasi");
    break;
  default:
    (void)snprintf(buffer, buffer_size, "Vana %u", (unsigned int)valve_id);
    break;
  }
}

static uint8_t FAULT_MGR_FindFirstFaultedValve(void) {
  for (uint8_t valve_id = 1U; valve_id <= VALVE_COUNT; valve_id++) {
    if (VALVES_GetFaultFlags(valve_id) != VALVE_FAULT_NONE) {
      return valve_id;
    }
  }

  return 0U;
}

static void FAULT_MGR_UpdateUserTexts(void) {
  uint8_t valve_id = 0U;
  uint8_t fault_flags = VALVE_FAULT_NONE;
  uint8_t interlock_source_id = 0U;
  char valve_name[24] = {0};
  char source_name[24] = {0};

  if (g_fault.active == 0U) {
    (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                   "Aktif alarm yok. Sistem hazir.");
    (void)snprintf(g_fault.action_text, sizeof(g_fault.action_text),
                   "Sulamayi baslatabilirsiniz.");
    return;
  }

  valve_id = FAULT_MGR_FindFirstFaultedValve();
  if (valve_id != 0U) {
    fault_flags = VALVES_GetFaultFlags(valve_id);
    interlock_source_id = VALVES_GetInterlockSource(valve_id);
    FAULT_MGR_CopyValveLabel(valve_id, valve_name, sizeof(valve_name));
  }

  if (interlock_source_id != 0U) {
    FAULT_MGR_CopyValveLabel(interlock_source_id, source_name,
                             sizeof(source_name));
  }

  switch (g_fault.last_error) {
  case ERR_PH_SENSOR_FAULT:
    (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                   "pH olcumu alinmiyor veya veri gec kaldi.");
    break;
  case ERR_EC_SENSOR_FAULT:
    (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                   "EC olcumu alinmiyor veya veri gec kaldi.");
    break;
  case ERR_PH_OUT_OF_RANGE:
    (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                   "Asit tanki bos olabilir veya pH hedefe inmiyor. Asit hattini kontrol edin.");
    break;
  case ERR_EC_OUT_OF_RANGE:
    (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                   "EC hedef araligin disina cikti.");
    break;
  case ERR_VALVE_STUCK:
    if ((fault_flags & VALVE_FAULT_DISABLED) != 0U && valve_name[0] != '\0') {
      (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                     "%s devre disi. Ayardan etkinlestirin.", valve_name);
    } else if ((fault_flags & VALVE_FAULT_PARCEL_INTERLOCK) != 0U &&
               valve_name[0] != '\0' && source_name[0] != '\0') {
      (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                     "%s acik oldugu icin %s acilmadi.", source_name,
                     valve_name);
    } else if ((fault_flags & VALVE_FAULT_DOSING_INTERLOCK) != 0U &&
               valve_name[0] != '\0') {
      (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                     "%s icin dozaj kilidi aktif. Asit ve gubre ayni anda acilamaz.",
                     valve_name);
    } else if ((fault_flags & VALVE_FAULT_STATE_MISMATCH) != 0U &&
               valve_name[0] != '\0') {
      (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                     "%s komut aldi ama beklenen duruma gecmedi.", valve_name);
    } else if (valve_id <= PARCEL_VALVE_COUNT && valve_id != 0U) {
      (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                     "%u numarali parsel vanasi cevap vermiyor. Kablo ve bobini kontrol edin.",
                     (unsigned int)valve_id);
    } else if (valve_id == DOSING_VALVE_ACID_ID) {
      (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                     "Asit vanasi cevap vermiyor. Tank, kablo ve bobini kontrol edin.");
    } else {
      (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                     "Bir vana komuta cevap vermedi. Kablo, bobin veya surucuyu kontrol edin.");
    }
    break;
  case ERR_TIMEOUT:
    if (g_fault.recommended_state == CTRL_STATE_EMERGENCY_STOP) {
      (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                     "Acil durus aktif. Tum vanalar guvenli durum icin kapatildi.");
    } else {
      (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                     "Islem izin verilen sureyi asti.");
    }
    break;
  case ERR_LOW_WATER_LEVEL:
    (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                   "Su seviyesi dusuk. Emis veya depo seviyesini kontrol edin.");
    break;
  case ERR_OVERCURRENT:
    (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                   "Surucude asiri akim algilandi.");
    break;
  case ERR_COMMUNICATION:
    if (RTC_IsInitialized() == 0U) {
      (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                     "RTC ayari yok. Zamanli program baslatilamiyor.");
    } else if ((g_fault.valve_error_mask &
         (VALVE_FAULT_DRIVER | VALVE_FAULT_TIMEOUT)) != 0U) {
      (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                     "Vana surucusu veya I2C genisletici cevap vermiyor.");
    } else {
      (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                     "Cihazlar arasi haberlesme kesildi.");
    }
    break;
  case ERR_NONE:
  default:
    (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                   "Aktif alarm yok. Sistem hazir.");
    break;
  }

  if (g_fault.manual_ack_required != 0U) {
    if (g_fault.last_error == ERR_COMMUNICATION && RTC_IsInitialized() == 0U) {
      (void)snprintf(g_fault.action_text, sizeof(g_fault.action_text),
                     "Saat ve tarihi ayarlayin, sonra ONAYLA tusuna basin.");
    } else {
      (void)snprintf(g_fault.action_text, sizeof(g_fault.action_text),
                     "Sahayi kontrol edin, sonra ONAYLA tusuna basin.");
    }
  } else if (g_fault.active != 0U) {
    (void)snprintf(g_fault.action_text, sizeof(g_fault.action_text),
                   "Sorun giderildiyse RESET ile alarmi temizleyin.");
  } else {
    (void)snprintf(g_fault.action_text, sizeof(g_fault.action_text),
                   "Sulamayi baslatabilirsiniz.");
  }
}

void FAULT_MGR_Init(void) { FAULT_MGR_Reset(); }

void FAULT_MGR_Reset(void) {
  memset(&g_fault, 0, sizeof(g_fault));
  g_fault.last_error = ERR_NONE;
  g_fault.recommended_state = CTRL_STATE_IDLE;
  (void)snprintf(g_fault.alarm_text, sizeof(g_fault.alarm_text), "READY");
  FAULT_MGR_UpdateUserTexts();
}

void FAULT_MGR_Raise(error_code_t error, uint8_t latched,
                     uint8_t manual_ack_required,
                     control_state_t recommended_state) {
  if (error == ERR_NONE) {
    return;
  }

  g_fault.active = 1U;
  g_fault.latched = (latched != 0U) ? 1U : 0U;
  g_fault.manual_ack_required = (manual_ack_required != 0U) ? 1U : 0U;
  g_fault.acknowledged = 0U;
  g_fault.last_error = error;
  g_fault.recommended_state = recommended_state;
  FAULT_MGR_SetAlarmFromError(error);
}

void FAULT_MGR_Clear(error_code_t error) {
  if (error != ERR_NONE && g_fault.last_error != error) {
    return;
  }

  g_fault.active = 0U;
  g_fault.latched = 0U;
  g_fault.manual_ack_required = 0U;
  g_fault.acknowledged = 0U;
  g_fault.last_error = ERR_NONE;
  g_fault.recommended_state = CTRL_STATE_IDLE;
  (void)snprintf(g_fault.alarm_text, sizeof(g_fault.alarm_text), "READY");
  FAULT_MGR_UpdateUserTexts();
}

void FAULT_MGR_ClearAll(void) { FAULT_MGR_Clear(ERR_NONE); }

void FAULT_MGR_EnterEmergencyStop(error_code_t fallback_error) {
  if (g_fault.last_error == ERR_NONE) {
    g_fault.last_error =
        (fallback_error == ERR_NONE) ? ERR_TIMEOUT : fallback_error;
  }

  g_fault.active = 1U;
  g_fault.latched = 1U;
  g_fault.manual_ack_required = 1U;
  g_fault.acknowledged = 0U;
  g_fault.recommended_state = CTRL_STATE_EMERGENCY_STOP;
  FAULT_MGR_SetAlarmFromError(g_fault.last_error);
}

void FAULT_MGR_Acknowledge(void) {
  if (g_fault.active == 0U) {
    return;
  }

  g_fault.acknowledged = 1U;
  g_fault.manual_ack_required = 0U;
  FAULT_MGR_UpdateUserTexts();
}

void FAULT_MGR_SetAlarmText(const char *text) {
  if (text == NULL || text[0] == '\0') {
    (void)snprintf(g_fault.alarm_text, sizeof(g_fault.alarm_text), "READY");
    FAULT_MGR_UpdateUserTexts();
    return;
  }

  (void)snprintf(g_fault.alarm_text, sizeof(g_fault.alarm_text), "%s", text);
  if (g_fault.active == 0U) {
    (void)snprintf(g_fault.detail_text, sizeof(g_fault.detail_text),
                   "Son islem tamamlandi.");
    (void)snprintf(g_fault.action_text, sizeof(g_fault.action_text),
                   "Yeni komut icin sistem hazir.");
  }
}

void FAULT_MGR_SetValveErrorMask(uint8_t valve_error_mask) {
  g_fault.valve_error_mask = valve_error_mask;
  if (g_fault.active != 0U) {
    FAULT_MGR_UpdateUserTexts();
  }
}

uint8_t FAULT_MGR_GetValveErrorMask(void) { return g_fault.valve_error_mask; }

uint8_t FAULT_MGR_HasActiveFault(void) { return g_fault.active; }

uint8_t FAULT_MGR_IsLatched(void) { return g_fault.latched; }

uint8_t FAULT_MGR_RequiresManualAck(void) {
  return g_fault.manual_ack_required;
}

uint8_t FAULT_MGR_IsAcknowledged(void) { return g_fault.acknowledged; }

uint8_t FAULT_MGR_CanReset(void) {
  return ((g_fault.active != 0U) && (g_fault.manual_ack_required == 0U)) ? 1U
                                                                          : 0U;
}

error_code_t FAULT_MGR_GetLastError(void) { return g_fault.last_error; }

control_state_t FAULT_MGR_GetRecommendedState(void) {
  return g_fault.recommended_state;
}

const char *FAULT_MGR_GetAlarmText(void) { return g_fault.alarm_text; }

const char *FAULT_MGR_GetDetailText(void) { return g_fault.detail_text; }

const char *FAULT_MGR_GetActionText(void) { return g_fault.action_text; }

const char *FAULT_MGR_GetErrorString(error_code_t error) {
  switch (error) {
  case ERR_PH_SENSOR_FAULT:
    return "PH SENSOR";
  case ERR_EC_SENSOR_FAULT:
    return "EC SENSOR";
  case ERR_PH_OUT_OF_RANGE:
    return "PH RANGE";
  case ERR_EC_OUT_OF_RANGE:
    return "EC RANGE";
  case ERR_VALVE_STUCK:
    return "VALVE";
  case ERR_TIMEOUT:
    return "TIMEOUT";
  case ERR_LOW_WATER_LEVEL:
    return "LOW WATER";
  case ERR_OVERCURRENT:
    return "OVERCURRENT";
  case ERR_COMMUNICATION:
    return "COMM";
  case ERR_NONE:
  default:
    return "READY";
  }
}

void FAULT_MGR_GetStatus(fault_manager_status_t *status) {
  if (status == NULL) {
    return;
  }

  *status = g_fault;
}
