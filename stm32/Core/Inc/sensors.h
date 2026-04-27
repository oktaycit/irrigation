/**
 ******************************************************************************
 * @file           : sensors.h
 * @brief          : pH ve EC Sensör Okuma Driver
 ******************************************************************************
 */

#ifndef __SENSORS_H
#define __SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"

/* Sensor Interface Selection -----------------------------------------------*/
#define SENSORS_INTERFACE_MODBUS 1U
#define SENSORS_POLL_INTERVAL_MS 250U

/* Modbus Sensor Configuration ----------------------------------------------*/
#if SENSORS_INTERFACE_MODBUS
#define SENSORS_MODBUS_UART_INSTANCE USART1
#define SENSORS_MODBUS_BAUDRATE 19200U
#define SENSORS_MODBUS_READ_HOLDING_REGS 0x03U

#define EC_SENSOR_MODBUS_ADDRESS 0x01U
#define EC_SENSOR_MODBUS_START_REG 0x0000U
#define EC_SENSOR_MODBUS_REG_COUNT 0x0002U

#define PH_SENSOR_MODBUS_ADDRESS 0x02U
#define PH_SENSOR_MODBUS_START_REG 0x0000U
#define PH_SENSOR_MODBUS_REG_COUNT 0x0001U

/*
 * Example projedeki register yapisina gore:
 * EC  = register 1
 * Temp = register 2
 * pH  = register 1
 *
 * Sensor register olcekleri farkliysa bu carpanlari guncelleyin.
 */
#define EC_SENSOR_MODBUS_SCALE 100.0f
#define PH_SENSOR_MODBUS_SCALE 100.0f
#define TEMP_SENSOR_MODBUS_SCALE 10.0f

#define SENSORS_MODBUS_UART_TIMEOUT_MS 20U
#define SENSORS_MODBUS_RESPONSE_TIMEOUT_MS 20U

#define SENSORS_RS485_USE_DE_PIN 1U
#define SENSORS_RS485_DE_PORT GPIOB
#define SENSORS_RS485_DE_PIN GPIO_PIN_11
#define SENSORS_RS485_TX_ACTIVE GPIO_PIN_SET
#define SENSORS_RS485_RX_ACTIVE GPIO_PIN_RESET
#endif

/* ADC Configuration --------------------------------------------------------*/
#define PH_ADC_CHANNEL ADC_CHANNEL_0
#define PH_ADC_PIN GPIO_PIN_0
#define PH_ADC_PORT GPIOA

#define EC_ADC_CHANNEL ADC_CHANNEL_1
#define EC_ADC_PIN GPIO_PIN_1
#define EC_ADC_PORT GPIOA

#define ADC_RESOLUTION ADC_RESOLUTION_12B
#define ADC_MAX_VALUE 4095U
#define ADC_REF_VOLTAGE 3.3f

/* Sensor Ranges ------------------------------------------------------------*/
#define PH_MIN_VALUE 0.0f
#define PH_MAX_VALUE 14.0f
#define EC_MIN_VALUE 0.0f
#define EC_MAX_VALUE 20.0f /* mS/cm */

/* Sensor Status ------------------------------------------------------------*/
#define SENSOR_OK 0U
#define SENSOR_ERROR 1U
#define SENSOR_OUT_OF_RANGE 2U
#define SENSOR_NOT_CALIBRATED 3U
#define SENSOR_TIMEOUT 4U
#define SENSOR_COMM_ERROR 5U

/* Filter Configuration -----------------------------------------------------*/
#define FILTER_SAMPLES 10U /* Moving average samples */
#define FILTER_ENABLED 1U

/* Calibration Points -------------------------------------------------------*/
#define PH_CAL_POINTS 3U /* pH 4.01, 7.00, 10.01 */
#define EC_CAL_POINTS 2U /* 1413 µS/cm, 12.88 mS/cm */

/* pH Calibration Solutions -------------------------------------------------*/
#define PH_CAL_1_VALUE 4.01f
#define PH_CAL_2_VALUE 7.00f
#define PH_CAL_3_VALUE 10.01f

/* EC Calibration Solutions -------------------------------------------------*/
#define EC_CAL_1_VALUE 1.413f /* mS/cm */
#define EC_CAL_2_VALUE 12.88f /* mS/cm */

/* Temperature Compensation -------------------------------------------------*/
#define TEMP_COMPENSATION_ENABLED 1U
#define TEMP_REFERENCE 25.0f /* 25°C reference */

/* Sensor Data Structures ---------------------------------------------------*/
typedef struct {
  float ph_value;
  float raw_voltage;
  uint16_t raw_adc;
  uint8_t status;
  uint8_t is_calibrated;
  float temperature;
  uint32_t last_read_time;
} ph_sensor_data_t;

typedef struct {
  float ec_value; /* mS/cm */
  float raw_voltage;
  uint16_t raw_adc;
  uint8_t status;
  uint8_t is_calibrated;
  float temperature;
  uint32_t last_read_time;
} ec_sensor_data_t;

/* Calibration Data Structures ----------------------------------------------*/
typedef struct {
  float adc_values[PH_CAL_POINTS];
  float ph_values[PH_CAL_POINTS];
  float slope;
  float offset;
  uint8_t points_count;
  uint8_t is_valid;
  uint32_t calibration_date;
} ph_calibration_t;

typedef struct {
  float adc_values[EC_CAL_POINTS];
  float ec_values[EC_CAL_POINTS];
  float slope;
  float offset;
  uint8_t points_count;
  uint8_t is_valid;
  uint32_t calibration_date;
} ec_calibration_t;

/* Sensor Handle Structure --------------------------------------------------*/
typedef struct {
  ph_sensor_data_t ph;
  ec_sensor_data_t ec;
  ph_calibration_t ph_cal;
  ec_calibration_t ec_cal;
  uint16_t adc_buffer[FILTER_SAMPLES];
  uint8_t buffer_index;
} sensors_handle_t;

/* Sensor Functions ---------------------------------------------------------*/
void SENSORS_Init(void);
void SENSORS_StartContinuous(void);
void SENSORS_StopContinuous(void);
void SENSORS_Process(void);

/* pH Sensor Functions ------------------------------------------------------*/
uint8_t PH_Read(ph_sensor_data_t *data);
uint8_t PH_CalibratePoint(float solution_value);
uint8_t PH_CalibrationComplete(void);
void PH_SetCalibration(const ph_calibration_t *cal);
void PH_GetCalibration(ph_calibration_t *cal);
float PH_GetValue(void);
float PH_GetVoltage(void);
uint8_t PH_GetStatus(void);

/* EC Sensor Functions ------------------------------------------------------*/
uint8_t EC_Read(ec_sensor_data_t *data);
uint8_t EC_CalibratePoint(float solution_value);
uint8_t EC_CalibrationComplete(void);
void EC_SetCalibration(const ec_calibration_t *cal);
void EC_GetCalibration(ec_calibration_t *cal);
float EC_GetValue(void);
float EC_GetVoltage(void);
uint8_t EC_GetStatus(void);

/* Temperature Functions ----------------------------------------------------*/
void SENSORS_SetTemperature(float temp);
float SENSORS_GetTemperature(void);
float EC_TemperatureCompensate(float ec_value, float temperature);
uint32_t PH_GetLastReadTime(void);
uint32_t EC_GetLastReadTime(void);

/* Filter Functions ---------------------------------------------------------*/
uint16_t SENSORS_FilterADC(uint16_t new_value);
float SENSORS_MovingAverage(float *buffer, uint8_t size);

/* Callbacks ----------------------------------------------------------------*/
void SENSORS_ADC_CompleteCallback(void);

#ifdef __cplusplus
}
#endif

#endif /* __SENSORS_H */
