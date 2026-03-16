/**
 ******************************************************************************
 * @file           : sensors.c
 * @brief          : pH ve EC sensör sürücüsü
 ******************************************************************************
 */

#include "main.h"
#include <string.h>

#if SENSORS_INTERFACE_MODBUS
extern UART_HandleTypeDef huart1;
#else
extern ADC_HandleTypeDef hadc1;
#endif

/* Sensor Handle */
static sensors_handle_t h_sensors = {0};
static uint32_t g_last_poll_time = 0;

#if SENSORS_INTERFACE_MODBUS
#define SENSORS_MODBUS_MAX_RESPONSE_SIZE 16U

typedef enum {
    SENSORS_MODBUS_QUERY_NONE = 0,
    SENSORS_MODBUS_QUERY_EC,
    SENSORS_MODBUS_QUERY_PH
} sensors_modbus_query_t;

typedef struct {
    volatile sensors_modbus_query_t active_query;
    volatile uint8_t tx_done;
    volatile uint8_t rx_done;
    volatile uint8_t error_status;
    volatile uint16_t response_size;
    uint8_t tx_buffer[8];
    uint8_t rx_buffer[SENSORS_MODBUS_MAX_RESPONSE_SIZE];
    uint32_t transaction_start_tick;
} sensors_modbus_context_t;

static sensors_modbus_context_t h_modbus = {0};
#endif

#if !SENSORS_INTERFACE_MODBUS
static uint8_t SENSORS_ReadChannel(uint32_t channel, uint16_t *raw_value);
#endif
static void SENSORS_UpdatePHMeasurement(void);
static void SENSORS_UpdateECMeasurement(void);
static uint8_t SENSORS_RunLinearCalibration(float *x_values, float *y_values,
                                            uint8_t points_count, float *slope,
                                            float *offset);

#if SENSORS_INTERFACE_MODBUS
static void SENSORS_ModbusInitInterface(void);
static void SENSORS_ModbusSetDirection(GPIO_PinState state);
static void SENSORS_ModbusFlushRx(void);
static uint16_t SENSORS_ModbusCRC16(const uint8_t *data, uint16_t length);
static uint8_t SENSORS_ModbusStartTransaction(sensors_modbus_query_t query_type);
static void SENSORS_ModbusScheduleCycle(void);
static void SENSORS_ModbusHandleTransactionResult(void);
static void SENSORS_ModbusResetTransaction(void);
static uint8_t SENSORS_ModbusProcessResponse(sensors_modbus_query_t query_type);
#endif

/**
 * @brief  Initialize Sensors
 */
void SENSORS_Init(void) {
    memset(&h_sensors, 0, sizeof(h_sensors));

    h_sensors.ph.status = SENSOR_ERROR;
    h_sensors.ec.status = SENSOR_ERROR;
    h_sensors.ph.temperature = TEMP_REFERENCE;
    h_sensors.ec.temperature = TEMP_REFERENCE;

#if SENSORS_INTERFACE_MODBUS
    memset(&h_modbus, 0, sizeof(h_modbus));
    SENSORS_ModbusInitInterface();
    g_last_poll_time = 0;
    SENSORS_Process();
#else
    SENSORS_UpdateECMeasurement();
    SENSORS_UpdatePHMeasurement();
    g_last_poll_time = HAL_GetTick();
#endif
}

/**
 * @brief  Start continuous sensor polling
 */
void SENSORS_StartContinuous(void) {
    g_last_poll_time = 0;
    SENSORS_Process();
}

/**
 * @brief  Stop continuous reading
 */
void SENSORS_StopContinuous(void) {
#if !SENSORS_INTERFACE_MODBUS
    HAL_ADC_Stop(&hadc1);
#else
    SENSORS_ModbusSetDirection(SENSORS_RS485_RX_ACTIVE);
#endif
}

/**
 * @brief  Periodic sensor task
 */
void SENSORS_Process(void) {
    uint32_t now = HAL_GetTick();

#if SENSORS_INTERFACE_MODBUS
    if (h_modbus.active_query != SENSORS_MODBUS_QUERY_NONE) {
        if (h_modbus.error_status != SENSOR_OK) {
            SENSORS_ModbusHandleTransactionResult();
        } else if ((h_modbus.tx_done != 0U) && (h_modbus.rx_done != 0U)) {
            SENSORS_ModbusHandleTransactionResult();
        } else if ((now - h_modbus.transaction_start_tick) >=
                   SENSORS_MODBUS_RESPONSE_TIMEOUT_MS) {
            h_modbus.error_status = SENSOR_TIMEOUT;
            SENSORS_ModbusHandleTransactionResult();
        }
        return;
    }
#endif

    if ((now - g_last_poll_time) < SENSORS_POLL_INTERVAL_MS) {
        return;
    }

    g_last_poll_time = now;
#if SENSORS_INTERFACE_MODBUS
    SENSORS_ModbusScheduleCycle();
#else
    SENSORS_UpdateECMeasurement();
    SENSORS_UpdatePHMeasurement();
#endif
}

/**
 * @brief  ADC DMA Complete Callback
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    (void)hadc;
}

/**
 * @brief  Read and process pH value
 */
uint8_t PH_Read(ph_sensor_data_t *data) {
    float measurement;

#if SENSORS_INTERFACE_MODBUS
    measurement = data->ph_value;
    if (h_sensors.ph_cal.is_valid) {
        data->ph_value = h_sensors.ph_cal.slope * measurement + h_sensors.ph_cal.offset;
    }
#else
    measurement = data->raw_voltage;
    if (!h_sensors.ph_cal.is_valid) {
        data->ph_value = data->raw_voltage * 14.0f / 3.3f;
    } else {
        data->ph_value = h_sensors.ph_cal.slope * measurement + h_sensors.ph_cal.offset;
    }
#endif

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
    float measurement;

#if SENSORS_INTERFACE_MODBUS
    measurement = data->ec_value;
    if (h_sensors.ec_cal.is_valid) {
        data->ec_value = h_sensors.ec_cal.slope * measurement + h_sensors.ec_cal.offset;
    }
#else
    measurement = data->raw_voltage;
    if (!h_sensors.ec_cal.is_valid) {
        data->ec_value = data->raw_voltage * 20.0f / 3.3f;
    } else {
        data->ec_value = h_sensors.ec_cal.slope * measurement + h_sensors.ec_cal.offset;
    }

    data->ec_value = EC_TemperatureCompensate(data->ec_value, data->temperature);
#endif

    if (data->ec_value < EC_MIN_VALUE || data->ec_value > EC_MAX_VALUE) {
        data->status = SENSOR_OUT_OF_RANGE;
    } else {
        data->status = SENSOR_OK;
    }

    return data->status;
}

/**
 * @brief  EC Temperature Compensation
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
    if (idx >= PH_CAL_POINTS) {
        return 0;
    }

#if SENSORS_INTERFACE_MODBUS
    h_sensors.ph_cal.adc_values[idx] = h_sensors.ph.ph_value;
#else
    h_sensors.ph_cal.adc_values[idx] = h_sensors.ph.raw_voltage;
#endif
    h_sensors.ph_cal.ph_values[idx] = solution_value;
    h_sensors.ph_cal.points_count++;

    return 1;
}

/**
 * @brief  Calculate pH calibration slope and offset
 */
uint8_t PH_CalibrationComplete(void) {
    if (!SENSORS_RunLinearCalibration(h_sensors.ph_cal.adc_values,
                                      h_sensors.ph_cal.ph_values,
                                      h_sensors.ph_cal.points_count,
                                      &h_sensors.ph_cal.slope,
                                      &h_sensors.ph_cal.offset)) {
        return 0;
    }

    h_sensors.ph_cal.is_valid = 1;
    h_sensors.ph_cal.calibration_date = HAL_GetTick();
    return 1;
}

/**
 * @brief  Start Calibration for EC
 */
uint8_t EC_CalibratePoint(float solution_value) {
    uint8_t idx = h_sensors.ec_cal.points_count;
    if (idx >= EC_CAL_POINTS) {
        return 0;
    }

#if SENSORS_INTERFACE_MODBUS
    h_sensors.ec_cal.adc_values[idx] = h_sensors.ec.ec_value;
#else
    h_sensors.ec_cal.adc_values[idx] = h_sensors.ec.raw_voltage;
#endif
    h_sensors.ec_cal.ec_values[idx] = solution_value;
    h_sensors.ec_cal.points_count++;

    return 1;
}

/**
 * @brief  Calculate EC calibration slope and offset
 */
uint8_t EC_CalibrationComplete(void) {
    if (!SENSORS_RunLinearCalibration(h_sensors.ec_cal.adc_values,
                                      h_sensors.ec_cal.ec_values,
                                      h_sensors.ec_cal.points_count,
                                      &h_sensors.ec_cal.slope,
                                      &h_sensors.ec_cal.offset)) {
        return 0;
    }

    h_sensors.ec_cal.is_valid = 1;
    h_sensors.ec_cal.calibration_date = HAL_GetTick();
    return 1;
}

void PH_SetCalibration(const ph_calibration_t *cal) {
    if (cal == NULL) {
        return;
    }

    h_sensors.ph_cal = *cal;
}

void PH_GetCalibration(ph_calibration_t *cal) {
    if (cal == NULL) {
        return;
    }

    *cal = h_sensors.ph_cal;
}

void EC_SetCalibration(const ec_calibration_t *cal) {
    if (cal == NULL) {
        return;
    }

    h_sensors.ec_cal = *cal;
}

void EC_GetCalibration(ec_calibration_t *cal) {
    if (cal == NULL) {
        return;
    }

    *cal = h_sensors.ec_cal;
}

float PH_GetValue(void) {
    SENSORS_Process();
    return h_sensors.ph.ph_value;
}

float EC_GetValue(void) {
    SENSORS_Process();
    return h_sensors.ec.ec_value;
}

float PH_GetVoltage(void) {
    SENSORS_Process();
    return h_sensors.ph.raw_voltage;
}

float EC_GetVoltage(void) {
    SENSORS_Process();
    return h_sensors.ec.raw_voltage;
}

uint8_t PH_GetStatus(void) {
    SENSORS_Process();
    return h_sensors.ph.status;
}

uint8_t EC_GetStatus(void) {
    SENSORS_Process();
    return h_sensors.ec.status;
}

void SENSORS_SetTemperature(float temp) {
    h_sensors.ph.temperature = temp;
    h_sensors.ec.temperature = temp;
}

float SENSORS_GetTemperature(void) {
    SENSORS_Process();
    return h_sensors.ec.temperature;
}

uint16_t SENSORS_FilterADC(uint16_t new_value) {
    h_sensors.adc_buffer[h_sensors.buffer_index] = new_value;
    h_sensors.buffer_index = (uint8_t)((h_sensors.buffer_index + 1U) % FILTER_SAMPLES);

    uint32_t sum = 0;
    for (uint8_t i = 0; i < FILTER_SAMPLES; i++) {
        sum += h_sensors.adc_buffer[i];
    }

    return (uint16_t)(sum / FILTER_SAMPLES);
}

float SENSORS_MovingAverage(float *buffer, uint8_t size) {
    if ((buffer == NULL) || (size == 0U)) {
        return 0.0f;
    }

    float sum = 0.0f;
    for (uint8_t i = 0; i < size; i++) {
        sum += buffer[i];
    }

    return sum / (float)size;
}

void SENSORS_ADC_CompleteCallback(void) {
    /* Reserved for future DMA based implementations. */
}

#if !SENSORS_INTERFACE_MODBUS
static uint8_t SENSORS_ReadChannel(uint32_t channel, uint16_t *raw_value) {
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel = channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return 0;
    }

    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        return 0;
    }

    if (HAL_ADC_PollForConversion(&hadc1, 10U) != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        return 0;
    }

    *raw_value = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return 1;
}
#endif

static void SENSORS_UpdatePHMeasurement(void) {
#if !SENSORS_INTERFACE_MODBUS
    if (!SENSORS_ReadChannel(PH_ADC_CHANNEL, &h_sensors.ph.raw_adc)) {
        h_sensors.ph.status = SENSOR_ERROR;
        return;
    }

    h_sensors.ph.raw_voltage =
        (float)h_sensors.ph.raw_adc * ADC_REF_VOLTAGE / ADC_MAX_VALUE;

    PH_Read(&h_sensors.ph);
    h_sensors.ph.last_read_time = HAL_GetTick();
#endif
}

static void SENSORS_UpdateECMeasurement(void) {
#if !SENSORS_INTERFACE_MODBUS
    if (!SENSORS_ReadChannel(EC_ADC_CHANNEL, &h_sensors.ec.raw_adc)) {
        h_sensors.ec.status = SENSOR_ERROR;
        return;
    }

    h_sensors.ec.raw_voltage =
        (float)h_sensors.ec.raw_adc * ADC_REF_VOLTAGE / ADC_MAX_VALUE;

    EC_Read(&h_sensors.ec);
    h_sensors.ec.last_read_time = HAL_GetTick();
#endif
}

static uint8_t SENSORS_RunLinearCalibration(float *x_values, float *y_values,
                                            uint8_t points_count, float *slope,
                                            float *offset) {
    float x = 0.0f;
    float y = 0.0f;
    float x2 = 0.0f;
    float xy = 0.0f;
    float denominator;

    if ((x_values == NULL) || (y_values == NULL) || (slope == NULL) ||
        (offset == NULL) || (points_count < 2U)) {
        return 0;
    }

    for (uint8_t i = 0; i < points_count; i++) {
        x += x_values[i];
        y += y_values[i];
        x2 += x_values[i] * x_values[i];
        xy += x_values[i] * y_values[i];
    }

    denominator = (points_count * x2) - (x * x);
    if ((denominator > -0.000001f) && (denominator < 0.000001f)) {
        return 0;
    }

    *slope = ((points_count * xy) - (x * y)) / denominator;
    *offset = (y - (*slope * x)) / points_count;
    return 1;
}

#if SENSORS_INTERFACE_MODBUS
static void SENSORS_ModbusInitInterface(void) {
    GPIO_InitTypeDef gpio_init = {0};

    huart1.Instance = SENSORS_MODBUS_UART_INSTANCE;
    huart1.Init.BaudRate = SENSORS_MODBUS_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;

    (void)HAL_UART_DeInit(&huart1);
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        h_sensors.ph.status = SENSOR_ERROR;
        h_sensors.ec.status = SENSOR_ERROR;
        return;
    }

#if SENSORS_RS485_USE_DE_PIN
    if (SENSORS_RS485_DE_PORT == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (SENSORS_RS485_DE_PORT == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    } else if (SENSORS_RS485_DE_PORT == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    } else if (SENSORS_RS485_DE_PORT == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }

    HAL_GPIO_WritePin(SENSORS_RS485_DE_PORT, SENSORS_RS485_DE_PIN,
                      SENSORS_RS485_RX_ACTIVE);

    gpio_init.Pin = SENSORS_RS485_DE_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(SENSORS_RS485_DE_PORT, &gpio_init);
#endif
}

static void SENSORS_ModbusSetDirection(GPIO_PinState state) {
#if SENSORS_RS485_USE_DE_PIN
    HAL_GPIO_WritePin(SENSORS_RS485_DE_PORT, SENSORS_RS485_DE_PIN, state);
#else
    (void)state;
#endif
}

static void SENSORS_ModbusFlushRx(void) {
    __HAL_UART_CLEAR_IDLEFLAG(&huart1);

    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != RESET) {
        (void)__HAL_UART_FLUSH_DRREGISTER(&huart1);
    }

    __HAL_UART_CLEAR_OREFLAG(&huart1);
}

static uint16_t SENSORS_ModbusCRC16(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFFU;

    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8U; bit++) {
            if ((crc & 0x0001U) != 0U) {
                crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
            } else {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

static uint8_t SENSORS_ModbusStartTransaction(sensors_modbus_query_t query_type) {
    uint8_t slave_address;
    uint16_t start_register;
    uint16_t register_count;
    uint16_t crc;

    switch (query_type) {
        case SENSORS_MODBUS_QUERY_EC:
            slave_address = EC_SENSOR_MODBUS_ADDRESS;
            start_register = EC_SENSOR_MODBUS_START_REG;
            register_count = EC_SENSOR_MODBUS_REG_COUNT;
            break;

        case SENSORS_MODBUS_QUERY_PH:
            slave_address = PH_SENSOR_MODBUS_ADDRESS;
            start_register = PH_SENSOR_MODBUS_START_REG;
            register_count = PH_SENSOR_MODBUS_REG_COUNT;
            break;

        default:
            return 0;
    }

    (void)HAL_UART_DMAStop(&huart1);
    SENSORS_ModbusFlushRx();
    memset((void *)h_modbus.rx_buffer, 0, sizeof(h_modbus.rx_buffer));

    h_modbus.active_query = query_type;
    h_modbus.tx_done = 0;
    h_modbus.rx_done = 0;
    h_modbus.error_status = SENSOR_OK;
    h_modbus.response_size = 0;
    h_modbus.transaction_start_tick = HAL_GetTick();

    h_modbus.tx_buffer[0] = slave_address;
    h_modbus.tx_buffer[1] = SENSORS_MODBUS_READ_HOLDING_REGS;
    h_modbus.tx_buffer[2] = (uint8_t)(start_register >> 8U);
    h_modbus.tx_buffer[3] = (uint8_t)(start_register & 0xFFU);
    h_modbus.tx_buffer[4] = (uint8_t)(register_count >> 8U);
    h_modbus.tx_buffer[5] = (uint8_t)(register_count & 0xFFU);
    crc = SENSORS_ModbusCRC16(h_modbus.tx_buffer, 6U);
    h_modbus.tx_buffer[6] = (uint8_t)(crc & 0xFFU);
    h_modbus.tx_buffer[7] = (uint8_t)(crc >> 8U);

    if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, h_modbus.rx_buffer,
                                     sizeof(h_modbus.rx_buffer)) != HAL_OK) {
        SENSORS_ModbusResetTransaction();
        return 0;
    }

    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);

    SENSORS_ModbusSetDirection(SENSORS_RS485_TX_ACTIVE);
    if (HAL_UART_Transmit_DMA(&huart1, h_modbus.tx_buffer,
                              sizeof(h_modbus.tx_buffer)) != HAL_OK) {
        SENSORS_ModbusResetTransaction();
        return 0;
    }

    return 1;
}

static void SENSORS_ModbusScheduleCycle(void) {
    if (!SENSORS_ModbusStartTransaction(SENSORS_MODBUS_QUERY_EC)) {
        h_sensors.ec.status = SENSOR_ERROR;
        h_sensors.ph.status = SENSOR_ERROR;
    }
}

static void SENSORS_ModbusHandleTransactionResult(void) {
    sensors_modbus_query_t completed_query = h_modbus.active_query;
    uint8_t status = h_modbus.error_status;

    if ((status == SENSOR_OK) && (h_modbus.rx_done != 0U)) {
        status = SENSORS_ModbusProcessResponse(completed_query);
    } else if (status == SENSOR_OK) {
        status = SENSOR_ERROR;
    }

    SENSORS_ModbusResetTransaction();

    if (completed_query == SENSORS_MODBUS_QUERY_EC) {
        if (status != SENSOR_OK) {
            h_sensors.ec.status = status;
        }

        if (!SENSORS_ModbusStartTransaction(SENSORS_MODBUS_QUERY_PH)) {
            h_sensors.ph.status = SENSOR_ERROR;
        }
    } else if (completed_query == SENSORS_MODBUS_QUERY_PH) {
        if (status != SENSOR_OK) {
            h_sensors.ph.status = status;
        }
    }
}

static void SENSORS_ModbusResetTransaction(void) {
    (void)HAL_UART_DMAStop(&huart1);
    SENSORS_ModbusSetDirection(SENSORS_RS485_RX_ACTIVE);
    h_modbus.active_query = SENSORS_MODBUS_QUERY_NONE;
    h_modbus.tx_done = 0;
    h_modbus.rx_done = 0;
    h_modbus.error_status = SENSOR_OK;
    h_modbus.response_size = 0;
}

static uint8_t SENSORS_ModbusProcessResponse(sensors_modbus_query_t query_type) {
    uint8_t slave_address;
    uint16_t register_count;
    uint8_t expected_length;
    uint16_t received_crc;
    uint16_t calculated_crc;

    switch (query_type) {
        case SENSORS_MODBUS_QUERY_EC:
            slave_address = EC_SENSOR_MODBUS_ADDRESS;
            register_count = EC_SENSOR_MODBUS_REG_COUNT;
            break;

        case SENSORS_MODBUS_QUERY_PH:
            slave_address = PH_SENSOR_MODBUS_ADDRESS;
            register_count = PH_SENSOR_MODBUS_REG_COUNT;
            break;

        default:
            return SENSOR_ERROR;
    }

    expected_length = (uint8_t)(5U + (register_count * 2U));
    if (h_modbus.response_size != expected_length) {
        return SENSOR_COMM_ERROR;
    }

    if ((h_modbus.rx_buffer[0] != slave_address) ||
        (h_modbus.rx_buffer[1] != SENSORS_MODBUS_READ_HOLDING_REGS) ||
        (h_modbus.rx_buffer[2] != (uint8_t)(register_count * 2U))) {
        return SENSOR_COMM_ERROR;
    }

    received_crc = (uint16_t)h_modbus.rx_buffer[expected_length - 2U] |
                   ((uint16_t)h_modbus.rx_buffer[expected_length - 1U] << 8U);
    calculated_crc = SENSORS_ModbusCRC16(h_modbus.rx_buffer,
                                         (uint16_t)(expected_length - 2U));
    if (received_crc != calculated_crc) {
        return SENSOR_COMM_ERROR;
    }

    if (query_type == SENSORS_MODBUS_QUERY_EC) {
        h_sensors.ec.raw_adc =
            (uint16_t)((uint16_t)h_modbus.rx_buffer[3] << 8U) |
            h_modbus.rx_buffer[4];
        h_sensors.ec.raw_voltage = 0.0f;
        h_sensors.ec.ec_value = (float)h_sensors.ec.raw_adc / EC_SENSOR_MODBUS_SCALE;
        h_sensors.ec.temperature =
            (float)(((uint16_t)h_modbus.rx_buffer[5] << 8U) |
                    h_modbus.rx_buffer[6]) /
            TEMP_SENSOR_MODBUS_SCALE;
        h_sensors.ph.temperature = h_sensors.ec.temperature;
        EC_Read(&h_sensors.ec);
        h_sensors.ec.last_read_time = HAL_GetTick();
    } else {
        h_sensors.ph.raw_adc =
            (uint16_t)((uint16_t)h_modbus.rx_buffer[3] << 8U) |
            h_modbus.rx_buffer[4];
        h_sensors.ph.raw_voltage = 0.0f;
        h_sensors.ph.ph_value = (float)h_sensors.ph.raw_adc / PH_SENSOR_MODBUS_SCALE;
        PH_Read(&h_sensors.ph);
        h_sensors.ph.last_read_time = HAL_GetTick();
    }

    return SENSOR_OK;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance != SENSORS_MODBUS_UART_INSTANCE) {
        return;
    }

    SENSORS_ModbusSetDirection(SENSORS_RS485_RX_ACTIVE);
    h_modbus.tx_done = 1U;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
    if (huart->Instance != SENSORS_MODBUS_UART_INSTANCE) {
        return;
    }

    h_modbus.response_size = size;
    h_modbus.rx_done = 1U;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance != SENSORS_MODBUS_UART_INSTANCE) {
        return;
    }

    h_modbus.error_status = SENSOR_COMM_ERROR;
    SENSORS_ModbusSetDirection(SENSORS_RS485_RX_ACTIVE);
}
#endif
