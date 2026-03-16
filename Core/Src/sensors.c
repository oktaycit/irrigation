/**
 ******************************************************************************
 * @file           : sensors.c
 * @brief          : pH ve EC Sensör Okuma Driver Implementation
 ******************************************************************************
 */

#include "main.h"
#include <math.h>

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

/* Sensor Handle */
static sensors_handle_t h_sensors = {0};

/* DMA Buffer (2 channels: pH and EC) */
static uint32_t adc_dma_buffer[2];

/**
 * @brief  Initialize Sensors
 */
void SENSORS_Init(void) {
    /* Handle initialization */
    h_sensors.ph.status = SENSOR_OK;
    h_sensors.ec.status = SENSOR_OK;
    h_sensors.ph.temperature = 25.0f;
    h_sensors.ec.temperature = 25.0f;

    /* Load calibration from EEPROM (placeholder) */
}

/**
 * @brief  Start Continuous reading with DMA
 */
void SENSORS_StartContinuous(void) {
    HAL_ADC_Start_DMA(&hadc1, adc_dma_buffer, 2);
}

/**
 * @brief  Stop Continuous reading
 */
void SENSORS_StopContinuous(void) {
    HAL_ADC_Stop_DMA(&hadc1);
}

/**
 * @brief  ADC DMA Complete Callback
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) {
        /* Store raw values */
        h_sensors.ph.raw_adc = (uint16_t)adc_dma_buffer[0];
        h_sensors.ec.raw_adc = (uint16_t)adc_dma_buffer[1];

        /* Calculate raw voltage */
        h_sensors.ph.raw_voltage = (float)h_sensors.ph.raw_adc * ADC_REF_VOLTAGE / ADC_MAX_VALUE;
        h_sensors.ec.raw_voltage = (float)h_sensors.ec.raw_adc * ADC_REF_VOLTAGE / ADC_MAX_VALUE;

        /* Process values (apply calibration and filter) */
        PH_Read(&h_sensors.ph);
        EC_Read(&h_sensors.ec);

        h_sensors.ph.last_read_time = HAL_GetTick();
        h_sensors.ec.last_read_time = HAL_GetTick();
    }
}

/**
 * @brief  Read and process pH value
 */
uint8_t PH_Read(ph_sensor_data_t *data) {
    /* Simple linear mapping if not calibrated */
    if (!h_sensors.ph_cal.is_valid) {
        /* Assuming 0V = 0 pH, 3.3V = 14 pH as fallback */
        data->ph_value = data->raw_voltage * 14.0f / 3.3f;
    } else {
        /* Apply calibration formula: y = mx + c */
        data->ph_value = h_sensors.ph_cal.slope * data->raw_voltage + h_sensors.ph_cal.offset;
    }

    /* Range check */
    if (data->ph_value < PH_MIN_VALUE || data->ph_value > PH_MAX_VALUE) {
        data->status = SENSOR_OUT_OF_RANGE;
    } else {
        data->status = SENSOR_OK;
    }

    return data->status;
}

/**
 * @brief  Read and process EC value
 */
uint8_t EC_Read(ec_sensor_data_t *data) {
    float raw_ec;

    /* Simple linear mapping if not calibrated */
    if (!h_sensors.ec_cal.is_valid) {
        /* Assuming 0V = 0 mS/cm, 3.3V = 20 mS/cm as fallback */
        raw_ec = data->raw_voltage * 20.0f / 3.3f;
    } else {
        /* Apply calibration formula: y = mx + c */
        raw_ec = h_sensors.ec_cal.slope * data->raw_voltage + h_sensors.ec_cal.offset;
    }

    /* Temperature compensation */
    data->ec_value = EC_TemperatureCompensate(raw_ec, data->temperature);

    /* Range check */
    if (data->ec_value < EC_MIN_VALUE || data->ec_value > EC_MAX_VALUE) {
        data->status = SENSOR_OUT_OF_RANGE;
    } else {
        data->status = SENSOR_OK;
    }

    return data->status;
}

/**
 * @brief  EC Temperature Compensation
 * Formula: EC25 = EC_T / (1 + alpha * (T - 25))
 * alpha = 0.019 (standard for water)
 */
float EC_TemperatureCompensate(float ec_value, float temperature) {
#if TEMP_COMPENSATION_ENABLED
    float alpha = 0.019f;
    return ec_value / (1.0f + alpha * (temperature - TEMP_REFERENCE));
#else
    return ec_value;
#endif
}

/**
 * @brief  Start Calibration for pH
 */
uint8_t PH_CalibratePoint(float solution_value) {
    uint8_t idx = h_sensors.ph_cal.points_count;
    if (idx >= PH_CAL_POINTS) return 0;

    h_sensors.ph_cal.adc_values[idx] = h_sensors.ph.raw_voltage;
    h_sensors.ph_cal.ph_values[idx] = solution_value;
    h_sensors.ph_cal.points_count++;

    return 1;
}

/**
 * @brief  Calculate pH calibration slope and offset
 */
uint8_t PH_CalibrationComplete(void) {
    if (h_sensors.ph_cal.points_count < 2) return 0;

    /* Simple Least Squares Linear Regression for 2 or 3 points */
    float x = 0, y = 0, x2 = 0, xy = 0;
    uint8_t n = h_sensors.ph_cal.points_count;

    for (int i = 0; i < n; i++) {
        x += h_sensors.ph_cal.adc_values[i];
        y += h_sensors.ph_cal.ph_values[i];
        x2 += h_sensors.ph_cal.adc_values[i] * h_sensors.ph_cal.adc_values[i];
        xy += h_sensors.ph_cal.adc_values[i] * h_sensors.ph_cal.ph_values[i];
    }

    h_sensors.ph_cal.slope = (n * xy - x * y) / (n * x2 - x * x);
    h_sensors.ph_cal.offset = (y - h_sensors.ph_cal.slope * x) / n;
    h_sensors.ph_cal.is_valid = 1;

    return 1;
}

/* EC Calibration and other helpers similar to PH... */

float PH_GetValue(void) { return h_sensors.ph.ph_value; }
float EC_GetValue(void) { return h_sensors.ec.ec_value; }
uint8_t PH_GetStatus(void) { return h_sensors.ph.status; }
uint8_t EC_GetStatus(void) { return h_sensors.ec.status; }

void SENSORS_SetTemperature(float temp) {
    h_sensors.ph.temperature = temp;
    h_sensors.ec.temperature = temp;
}
