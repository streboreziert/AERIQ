#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "driver/uart.h"

static const char *TAG = "AERIQ";

// ── Pin & bus config ────────────────────────────────────────────────
#define I2C_MASTER_SCL_IO   GPIO_NUM_5
#define I2C_MASTER_SDA_IO   GPIO_NUM_4
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_FREQ_HZ  100000

// Dedicated UART link to the second ESP32-C3 using the chip UART0 signals.
// In many schematics/module pinouts these appear as RXD0/TXD0 on pins 30/31.
// On ESP32-C3 those signals map to GPIO20/GPIO21.
#define DATA_UART_NUM       UART_NUM_0
#define DATA_UART_BAUD      115200
#define DATA_UART_TX_PIN    GPIO_NUM_21
#define DATA_UART_RX_PIN    GPIO_NUM_20
#define DATA_UART_BUF_SIZE  256

// ── SCD41 definitions ───────────────────────────────────────────────
// Periodic measurement interval = 5 000 ms
#define SCD41_ADDR              0x62
#define SCD41_MEAS_INTERVAL_MS  5000
#define SCD41_STOP_MSB          0x3F
#define SCD41_STOP_LSB          0x86
#define SCD41_START_MSB         0x21
#define SCD41_START_LSB         0xB1
#define SCD41_READY_MSB         0xE4
#define SCD41_READY_LSB         0xB8
#define SCD41_READ_MSB          0xEC
#define SCD41_READ_LSB          0x05

// ── BME280 definitions ─────────────────────────────────────────────
#define BME280_ADDR         0x76
#define BME280_CHIP_ID_REG  0xD0
#define BME280_RESET_REG    0xE0
#define BME280_RESET_VAL    0xB6
#define BME280_CTRL_HUM_REG 0xF2
#define BME280_CTRL_HUM_VAL 0x01
#define BME280_CTRL_MEAS_REG 0xF4
#define BME280_CTRL_MEAS_VAL 0x27
#define BME280_CONFIG_REG   0xF5
#define BME280_CONFIG_VAL   0x00
#define BME280_DATA_REG     0xF7
#define BME280_CALIB00_REG  0x88
#define BME280_CALIB26_REG  0xE1

// ── BME280 calibration data ────────────────────────────────────────
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
} bme280_calib_t;

static bme280_calib_t bme280_calib;
static int32_t t_fine;

static void data_uart_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = DATA_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = uart_driver_install(DATA_UART_NUM, DATA_UART_BUF_SIZE, 0, 0, NULL, 0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "UART driver install failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = uart_param_config(DATA_UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART param config failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = uart_set_pin(DATA_UART_NUM, DATA_UART_TX_PIN, DATA_UART_RX_PIN,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "UART pin config failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Data UART initialized on TX:GPIO%d RX:GPIO%d @ %d baud",
             DATA_UART_TX_PIN, DATA_UART_RX_PIN, DATA_UART_BAUD);
}

static void data_uart_send(bool scd_valid,
                           uint16_t co2,
                           float scd_t,
                           float scd_h,
                           bool bme_valid,
                           float bme_t,
                           float bme_h,
                           float bme_p)
{
    char frame[160];
    int len = snprintf(frame, sizeof(frame),
                       "AERIQ|SCD=%d|CO2=%u|ST=%.2f|SH=%.2f|BME=%d|BT=%.2f|BH=%.2f|BP=%.2f\\r\\n",
                       scd_valid ? 1 : 0,
                       co2,
                       scd_t,
                       scd_h,
                       bme_valid ? 1 : 0,
                       bme_t,
                       bme_h,
                       bme_p);

    if (len > 0) {
        uart_write_bytes(DATA_UART_NUM, frame, len);
    }
}

// ── I2C init ────────────────────────────────────────────────────────
static void i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C config failed: %s", esp_err_to_name(err));
        return;
    }
    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "I2C initialized on SDA:GPIO%d SCL:GPIO%d", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
}

// ── SCD41 CRC-8 (polynomial 0x31, init 0xFF) ────────────────────────
static uint8_t scd41_crc8(const uint8_t *data, int len)
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
    return crc;
}

static esp_err_t scd41_get_ready_status(uint16_t *ready_status)
{
    uint8_t cmd_ready[2] = {SCD41_READY_MSB, SCD41_READY_LSB};
    uint8_t ready_buf[3] = {0};

    esp_err_t ret = i2c_master_write_to_device(I2C_MASTER_NUM, SCD41_ADDR,
                                               cmd_ready, 2, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(2));

    ret = i2c_master_read_from_device(I2C_MASTER_NUM, SCD41_ADDR,
                                      ready_buf, 3, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        return ret;
    }

    if (scd41_crc8(ready_buf, 2) != ready_buf[2]) {
        return ESP_ERR_INVALID_CRC;
    }

    *ready_status = ((uint16_t)ready_buf[0] << 8) | ready_buf[1];
    return ESP_OK;
}

// ── SCD41 ───────────────────────────────────────────────────────────
static esp_err_t scd41_init(void)
{
    uint8_t stop_cmd[2] = {SCD41_STOP_MSB, SCD41_STOP_LSB};
    esp_err_t ret = i2c_master_write_to_device(I2C_MASTER_NUM, SCD41_ADDR,
                                               stop_cmd, 2, pdMS_TO_TICKS(1000));
    if (ret == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    uint8_t start_cmd[2] = {SCD41_START_MSB, SCD41_START_LSB};
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, SCD41_ADDR,
                                     start_cmd, 2, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        uint16_t ready_status = 0;
        esp_err_t status_ret = scd41_get_ready_status(&ready_status);
        if (status_ret == ESP_OK) {
            ESP_LOGW(TAG, "SCD41 start command rejected, sensor already responding (ready=0x%04X)", ready_status);
            return ESP_OK;
        }

        ESP_LOGE(TAG, "SCD41 start failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "SCD41 periodic measurement started (%d s interval)",
             SCD41_MEAS_INTERVAL_MS / 1000);
    return ESP_OK;
}

// Returns ESP_OK, ESP_ERR_NOT_FINISHED if measurement not ready, or error.
static esp_err_t scd41_read(uint16_t *co2_ppm, float *temperature, float *humidity)
{
    // 1. get_data_ready_status (0xE4B8)
    uint16_t ready = 0;
    esp_err_t ret = scd41_get_ready_status(&ready);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SCD41 ready-status read failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if ((ready & 0x07FF) == 0)
        return ESP_ERR_NOT_FINISHED;

    // 2. read_measurement (0xEC05)
    uint8_t cmd_read[2] = {SCD41_READ_MSB, SCD41_READ_LSB};
    uint8_t data[9] = {0};

    ret = i2c_master_write_to_device(I2C_MASTER_NUM, SCD41_ADDR,
                                     cmd_read, 2, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SCD41 read cmd failed: %s", esp_err_to_name(ret));
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(2));

    ret = i2c_master_read_from_device(I2C_MASTER_NUM, SCD41_ADDR,
                                      data, 9, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SCD41 data read failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if (scd41_crc8(&data[0], 2) != data[2]) {
        ESP_LOGE(TAG, "SCD41 CO2 CRC error"); return ESP_ERR_INVALID_CRC;
    }
    if (scd41_crc8(&data[3], 2) != data[5]) {
        ESP_LOGE(TAG, "SCD41 temp CRC error"); return ESP_ERR_INVALID_CRC;
    }
    if (scd41_crc8(&data[6], 2) != data[8]) {
        ESP_LOGE(TAG, "SCD41 hum CRC error");  return ESP_ERR_INVALID_CRC;
    }

    *co2_ppm     = ((uint16_t)data[0] << 8) | data[1];
    uint16_t t_r = ((uint16_t)data[3] << 8) | data[4];
    uint16_t h_r = ((uint16_t)data[6] << 8) | data[7];

    *temperature = -45.0f + 175.0f * ((float)t_r / 65535.0f);
    *humidity    = 100.0f * ((float)h_r / 65535.0f);
    return ESP_OK;
}

// ── BME280 ──────────────────────────────────────────────────────────
static esp_err_t bme280_init(void)
{
    uint8_t chip_id_reg = BME280_CHIP_ID_REG;
    uint8_t chip_id;
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, BME280_ADDR,
                                                  &chip_id_reg, 1, &chip_id, 1, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read BME280 chip ID: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "BME280 Chip ID: 0x%02X (expected 0x60)", chip_id);

    // Soft reset
    uint8_t reset_cmd[2] = {BME280_RESET_REG, BME280_RESET_VAL};
    i2c_master_write_to_device(I2C_MASTER_NUM, BME280_ADDR, reset_cmd, 2, pdMS_TO_TICKS(1000));
    vTaskDelay(pdMS_TO_TICKS(10));

    // Read calibration: 0x88..0xA1 (26 bytes for T & P)
    uint8_t calib00_reg = BME280_CALIB00_REG;
    uint8_t c0[26];
    ret = i2c_master_write_read_device(I2C_MASTER_NUM, BME280_ADDR,
                                       &calib00_reg, 1, c0, 26, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BME280 calib read (0x88) failed: %s", esp_err_to_name(ret));
        return ret;
    }
    bme280_calib.dig_T1 = (uint16_t)(c0[1] << 8) | c0[0];
    bme280_calib.dig_T2 = (int16_t)((c0[3] << 8) | c0[2]);
    bme280_calib.dig_T3 = (int16_t)((c0[5] << 8) | c0[4]);
    bme280_calib.dig_P1 = (uint16_t)(c0[7] << 8) | c0[6];
    bme280_calib.dig_P2 = (int16_t)((c0[9] << 8) | c0[8]);
    bme280_calib.dig_P3 = (int16_t)((c0[11] << 8) | c0[10]);
    bme280_calib.dig_P4 = (int16_t)((c0[13] << 8) | c0[12]);
    bme280_calib.dig_P5 = (int16_t)((c0[15] << 8) | c0[14]);
    bme280_calib.dig_P6 = (int16_t)((c0[17] << 8) | c0[16]);
    bme280_calib.dig_P7 = (int16_t)((c0[19] << 8) | c0[18]);
    bme280_calib.dig_P8 = (int16_t)((c0[21] << 8) | c0[20]);
    bme280_calib.dig_P9 = (int16_t)((c0[23] << 8) | c0[22]);

    // Read dig_H1 from 0xA1
    uint8_t h1_reg = 0xA1;
    ret = i2c_master_write_read_device(I2C_MASTER_NUM, BME280_ADDR,
                                       &h1_reg, 1, &bme280_calib.dig_H1, 1, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) return ret;

    // Read calibration: 0xE1..0xE7 (7 bytes for H)
    uint8_t calib26_reg = BME280_CALIB26_REG;
    uint8_t c1[7];
    ret = i2c_master_write_read_device(I2C_MASTER_NUM, BME280_ADDR,
                                       &calib26_reg, 1, c1, 7, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BME280 calib read (0xE1) failed: %s", esp_err_to_name(ret));
        return ret;
    }
    bme280_calib.dig_H2 = (int16_t)((c1[1] << 8) | c1[0]);
    bme280_calib.dig_H3 = c1[2];
    bme280_calib.dig_H4 = (int16_t)((c1[3] << 4) | (c1[4] & 0x0F));
    bme280_calib.dig_H5 = (int16_t)((c1[5] << 4) | (c1[4] >> 4));
    bme280_calib.dig_H6 = (int8_t)c1[6];

    // Humidity oversampling x1 (must be written before ctrl_meas)
    uint8_t ctrl_hum[2] = {BME280_CTRL_HUM_REG, BME280_CTRL_HUM_VAL};
    i2c_master_write_to_device(I2C_MASTER_NUM, BME280_ADDR, ctrl_hum, 2, pdMS_TO_TICKS(1000));

    // Config: standby 0.5 ms, filter off
    uint8_t config[2] = {BME280_CONFIG_REG, BME280_CONFIG_VAL};
    i2c_master_write_to_device(I2C_MASTER_NUM, BME280_ADDR, config, 2, pdMS_TO_TICKS(1000));

    // ctrl_meas: temp osrs x1, press osrs x1, normal mode
    uint8_t ctrl_meas[2] = {BME280_CTRL_MEAS_REG, BME280_CTRL_MEAS_VAL};
    i2c_master_write_to_device(I2C_MASTER_NUM, BME280_ADDR, ctrl_meas, 2, pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "BME280 initialized (calibration loaded)");
    return ESP_OK;
}

// Bosch compensation: temperature -> deg C
static float bme280_compensate_temperature(int32_t adc_T)
{
    double var1 = ((double)adc_T / 16384.0 - (double)bme280_calib.dig_T1 / 1024.0) * (double)bme280_calib.dig_T2;
    double var2 = (((double)adc_T / 131072.0 - (double)bme280_calib.dig_T1 / 8192.0) *
                   ((double)adc_T / 131072.0 - (double)bme280_calib.dig_T1 / 8192.0)) * (double)bme280_calib.dig_T3;
    t_fine = (int32_t)(var1 + var2);
    return (float)((var1 + var2) / 5120.0);
}

// Bosch compensation: pressure -> hPa
static float bme280_compensate_pressure(int32_t adc_P)
{
    double var1 = ((double)t_fine / 2.0) - 64000.0;
    double var2 = var1 * var1 * (double)bme280_calib.dig_P6 / 32768.0;
    var2 = var2 + var1 * (double)bme280_calib.dig_P5 * 2.0;
    var2 = (var2 / 4.0) + ((double)bme280_calib.dig_P4 * 65536.0);
    var1 = ((double)bme280_calib.dig_P3 * var1 * var1 / 524288.0 + (double)bme280_calib.dig_P2 * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * (double)bme280_calib.dig_P1;
    if (var1 == 0.0) return 0.0f;
    double p = 1048576.0 - (double)adc_P;
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = (double)bme280_calib.dig_P9 * p * p / 2147483648.0;
    var2 = p * (double)bme280_calib.dig_P8 / 32768.0;
    p = p + (var1 + var2 + (double)bme280_calib.dig_P7) / 16.0;
    return (float)(p / 100.0);
}

// Bosch compensation: humidity -> %RH
static float bme280_compensate_humidity(int32_t adc_H)
{
    double var_H = ((double)t_fine) - 76800.0;
    if (var_H == 0.0) return 0.0f;
    var_H = (adc_H - ((double)bme280_calib.dig_H4 * 64.0 + ((double)bme280_calib.dig_H5 / 16384.0) * var_H)) *
            ((double)bme280_calib.dig_H2 / 65536.0 * (1.0 + (double)bme280_calib.dig_H6 / 67108864.0 * var_H *
            (1.0 + (double)bme280_calib.dig_H3 / 67108864.0 * var_H)));
    var_H = var_H * (1.0 - (double)bme280_calib.dig_H1 * var_H / 524288.0);
    if (var_H > 100.0) var_H = 100.0;
    if (var_H < 0.0) var_H = 0.0;
    return (float)var_H;
}

static esp_err_t bme280_read(float *temperature, float *pressure, float *humidity)
{
    uint8_t reg = BME280_DATA_REG;
    uint8_t data[8];
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, BME280_ADDR,
                                                  &reg, 1, data, 8, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) return ret;

    int32_t adc_P = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | ((int32_t)data[2] >> 4);
    int32_t adc_T = ((int32_t)data[3] << 12) | ((int32_t)data[4] << 4) | ((int32_t)data[5] >> 4);
    int32_t adc_H = ((int32_t)data[6] << 8)  | (int32_t)data[7];

    // Temperature must be first (sets t_fine for pressure & humidity)
    *temperature = bme280_compensate_temperature(adc_T);
    *pressure    = bme280_compensate_pressure(adc_P);
    *humidity    = bme280_compensate_humidity(adc_H);
    return ESP_OK;
}

// ── Sensor task ─────────────────────────────────────────────────────
// SCD41 measurement interval = 5 s. Wait 2 cycles (10 s) before first
// read to guarantee at least one complete measurement is available.
static void sensor_task(void *arg)
{
    ESP_LOGI(TAG, "Waiting 10 s for first SCD41 measurement...");
    vTaskDelay(pdMS_TO_TICKS(10000));

    while (1) {
        // --- SCD41 ---
        uint16_t co2 = 0;
        float scd_t = 0.0f, scd_h = 0.0f;
        esp_err_t scd_ret = scd41_read(&co2, &scd_t, &scd_h);
        bool scd_valid = (scd_ret == ESP_OK);

        if (scd_valid) {
            ESP_LOGI(TAG, "SCD41  | CO2: %4u ppm  Temp: %5.1f C  RH: %4.1f %%",
                     co2, scd_t, scd_h);
        } else if (scd_ret == ESP_ERR_NOT_FINISHED) {
            ESP_LOGW(TAG, "SCD41  | data not ready yet");
        } else {
            ESP_LOGE(TAG, "SCD41  | read error: %s", esp_err_to_name(scd_ret));
        }

        // --- BME280 ---
        float bme_t = 0.0f, bme_p = 0.0f, bme_h = 0.0f;
        esp_err_t bme_ret = bme280_read(&bme_t, &bme_p, &bme_h);
        bool bme_valid = (bme_ret == ESP_OK);

        if (bme_valid) {
            ESP_LOGI(TAG, "BME280 | Temp: %5.1f C  RH: %4.1f %%  Pressure: %7.2f hPa",
                     bme_t, bme_h, bme_p);
        } else {
            ESP_LOGE(TAG, "BME280 | read error: %s", esp_err_to_name(bme_ret));
        }

        data_uart_send(scd_valid, co2, scd_t, scd_h, bme_valid, bme_t, bme_h, bme_p);

        // Poll every 5 s — aligned with SCD41 output rate
        vTaskDelay(pdMS_TO_TICKS(SCD41_MEAS_INTERVAL_MS));
    }
}

// ── Entry point ─────────────────────────────────────────────────────
void app_main(void)
{
    ESP_LOGI(TAG, "AERIQ Starting...");
    data_uart_init();
    i2c_master_init();

    ESP_LOGI(TAG, "Initializing SCD41...");
    if (scd41_init() != ESP_OK)
        ESP_LOGE(TAG, "SCD41 init failed - continuing anyway");

    ESP_LOGI(TAG, "Initializing BME280...");
    if (bme280_init() != ESP_OK)
        ESP_LOGE(TAG, "BME280 init failed - continuing anyway");

    ESP_LOGI(TAG, "Starting sensor task...");
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
}

