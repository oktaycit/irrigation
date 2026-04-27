/**
 ******************************************************************************
 * @file           : dosing_controller.h
 * @brief          : pH/EC dozaj durum makinesi
 ******************************************************************************
 */

#ifndef __DOSING_CONTROLLER_H
#define __DOSING_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "irrigation_types.h"
#include "sensors.h"

typedef enum {
  DOSING_PHASE_IDLE = 0,
  DOSING_PHASE_PH,
  DOSING_PHASE_EC,
  DOSING_PHASE_MIXING,
  DOSING_PHASE_SETTLING
} dosing_phase_t;

typedef struct {
  dosing_phase_t phase;
  uint16_t active_mask;
  uint8_t pending_duty_percent;
  uint8_t active_duty_percent;
  dosing_phase_t last_completed_phase;
  uint8_t last_completed_duty_percent;
  control_state_t recommended_state;
  error_code_t last_error;
} dosing_controller_status_t;

void DOSING_CTRL_Init(void);
void DOSING_CTRL_Reset(void);
void DOSING_CTRL_Update(const ph_control_params_t *ph_params,
                        const ec_control_params_t *ec_params,
                        const ph_sensor_data_t *ph_data,
                        const ec_sensor_data_t *ec_data,
                        control_state_t resume_state);
void DOSING_CTRL_StartMixing(const ph_control_params_t *ph_params,
                             const ec_control_params_t *ec_params,
                             control_state_t resume_state);
void DOSING_CTRL_StopMixing(void);
uint8_t DOSING_CTRL_IsMixingComplete(const ph_control_params_t *ph_params,
                                     const ec_control_params_t *ec_params);
void DOSING_CTRL_StartSettling(control_state_t resume_state);
uint8_t DOSING_CTRL_IsSettlingComplete(const ph_control_params_t *ph_params,
                                       const ec_control_params_t *ec_params);
uint8_t DOSING_CTRL_IsBusy(void);
uint16_t DOSING_CTRL_GetActiveMask(void);
error_code_t DOSING_CTRL_GetLastError(void);
dosing_phase_t DOSING_CTRL_GetPhase(void);
control_state_t DOSING_CTRL_GetRecommendedState(void);
void DOSING_CTRL_GetStatus(dosing_controller_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* __DOSING_CONTROLLER_H */
