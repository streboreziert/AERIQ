#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static const char *TAG = "AERIQ_SIM";

// PC-side replacement for ESP-IDF types/macros to keep structure close to main.c
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_NOT_FINISHED 1
#define ESP_FAIL -1

#define ESP_LOGI(tag, fmt, ...) printf("I (%s) " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) printf("W (%s) " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, fmt, ...) printf("E (%s) " fmt "\n", tag, ##__VA_ARGS__)

// Keep these names/values aligned with firmware source for easier comparison.
#define DATA_UART_BAUD      115200
#define DATA_UART_BUF_SIZE  256
#define SCD41_MEAS_INTERVAL_MS 5000

static int g_max_iterations = 0;
static int g_iteration = 0;
static char g_uart_loopback_frame[160];

static float random_f(float min_v, float max_v)
{
    float unit = (float)rand() / (float)RAND_MAX;
    return min_v + unit * (max_v - min_v);
}

static uint16_t random_u16(uint16_t min_v, uint16_t max_v)
{
    return (uint16_t)(min_v + (rand() % (max_v - min_v + 1)));
}

static const char *esp_err_to_name(esp_err_t err)
{
    if (err == ESP_OK) return "ESP_OK";
    if (err == ESP_ERR_NOT_FINISHED) return "ESP_ERR_NOT_FINISHED";
    return "ESP_FAIL";
}

static void data_uart_init(void)
{
    ESP_LOGI(TAG, "Data UART initialized (LOCAL SIM) @ %d baud, buf=%d",
             DATA_UART_BAUD, DATA_UART_BUF_SIZE);
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
    (void)snprintf(g_uart_loopback_frame, sizeof(g_uart_loopback_frame),
                   "AERIQ|SCD=%d|CO2=%u|ST=%.2f|SH=%.2f|BME=%d|BT=%.2f|BH=%.2f|BP=%.2f\r\n",
                   scd_valid ? 1 : 0,
                   co2,
                   scd_t,
                   scd_h,
                   bme_valid ? 1 : 0,
                   bme_t,
                   bme_h,
                   bme_p);

    // Simulate UART TX->RX loopback by printing exactly what receiver would see.
    printf("UART RX <= %s", g_uart_loopback_frame);
}

static void wifi_init_sta(void)
{
    ESP_LOGI(TAG, "WiFi STA init done (LOCAL SIM stub)");
}

static void http_post_readings(uint16_t co2, float scd_t, float scd_h,
                               float bme_t, float bme_h, float bme_p)
{
    char json[256];
    (void)snprintf(json, sizeof(json),
                   "{\"temperature\":%.1f,\"humidity\":%.1f,\"co2\":%u,"
                   "\"pressure\":%.2f,\"bme_temperature\":%.1f,\"bme_humidity\":%.1f}",
                   scd_t, scd_h, co2, bme_p, bme_t, bme_h);
    ESP_LOGI(TAG, "HTTP POST (LOCAL SIM): %s", json);
}

static void i2c_master_init(void)
{
    ESP_LOGI(TAG, "I2C initialized (LOCAL SIM stub)");
}

static esp_err_t scd41_init(void)
{
    ESP_LOGI(TAG, "SCD41 periodic measurement started (%d s interval)",
             SCD41_MEAS_INTERVAL_MS / 1000);
    return ESP_OK;
}

// Returns ESP_OK, ESP_ERR_NOT_FINISHED if measurement not ready, or error.
static esp_err_t scd41_read(uint16_t *co2_ppm, float *temperature, float *humidity)
{
    // Emulate occasional "not ready" status similar to real sensor timing.
    if ((g_iteration % 7) == 0) {
        return ESP_ERR_NOT_FINISHED;
    }

    *co2_ppm = random_u16(420, 2000);
    *temperature = random_f(18.0f, 30.0f);
    *humidity = random_f(30.0f, 75.0f);
    return ESP_OK;
}

static esp_err_t bme280_init(void)
{
    ESP_LOGI(TAG, "BME280 initialized (LOCAL SIM calibration loaded)");
    return ESP_OK;
}

static esp_err_t bme280_read(float *temperature, float *pressure, float *humidity)
{
    *temperature = random_f(18.0f, 30.0f);
    *pressure = random_f(985.0f, 1035.0f);
    *humidity = random_f(30.0f, 75.0f);
    return ESP_OK;
}

static bool parse_uart_loopback(const char *frame)
{
    int scd = 0;
    int bme = 0;
    uint16_t co2 = 0;
    float st = 0.0f;
    float sh = 0.0f;
    float bt = 0.0f;
    float bh = 0.0f;
    float bp = 0.0f;

    int matched = sscanf(frame,
                         "AERIQ|SCD=%d|CO2=%hu|ST=%f|SH=%f|BME=%d|BT=%f|BH=%f|BP=%f",
                         &scd, &co2, &st, &sh, &bme, &bt, &bh, &bp);
    if (matched == 8) {
        ESP_LOGI(TAG,
                 "Parsed loopback -> SCD:%d CO2:%u ST:%.2f SH:%.2f BME:%d BT:%.2f BH:%.2f BP:%.2f",
                 scd, co2, st, sh, bme, bt, bh, bp);
        return true;
    }

    ESP_LOGW(TAG, "Frame parse failed (matched=%d): %s", matched, frame);
    return false;
}

// Sensor task (PC loop)
static void sensor_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "Waiting 1 s for first simulated SCD41 measurement...");
    usleep(1000U * 1000U);

    while (g_max_iterations == 0 || g_iteration < g_max_iterations) {
        g_iteration++;

        // --- SCD41 ---
        uint16_t co2 = 0;
        float scd_t = 0.0f;
        float scd_h = 0.0f;
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
        float bme_t = 0.0f;
        float bme_p = 0.0f;
        float bme_h = 0.0f;
        esp_err_t bme_ret = bme280_read(&bme_t, &bme_p, &bme_h);
        bool bme_valid = (bme_ret == ESP_OK);

        if (bme_valid) {
            ESP_LOGI(TAG, "BME280 | Temp: %5.1f C  RH: %4.1f %%  Pressure: %7.2f hPa",
                     bme_t, bme_h, bme_p);
        } else {
            ESP_LOGE(TAG, "BME280 | read error: %s", esp_err_to_name(bme_ret));
        }

        data_uart_send(scd_valid, co2, scd_t, scd_h, bme_valid, bme_t, bme_h, bme_p);
        (void)parse_uart_loopback(g_uart_loopback_frame);

        http_post_readings(co2, scd_t, scd_h, bme_t, bme_h, bme_p);

        usleep((unsigned int)SCD41_MEAS_INTERVAL_MS * 1000U);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "AERIQ Starting (LOCAL C SIM)...");

    data_uart_init();
    i2c_master_init();

    ESP_LOGI(TAG, "Initializing WiFi...");
    wifi_init_sta();

    ESP_LOGI(TAG, "Initializing SCD41...");
    if (scd41_init() != ESP_OK) {
        ESP_LOGE(TAG, "SCD41 init failed - continuing anyway");
    }

    ESP_LOGI(TAG, "Initializing BME280...");
    if (bme280_init() != ESP_OK) {
        ESP_LOGE(TAG, "BME280 init failed - continuing anyway");
    }

    ESP_LOGI(TAG, "Starting sensor task...");
    sensor_task(NULL);
}

int main(int argc, char **argv)
{
    // Usage: ./local_simulator [iterations]
    // iterations=0 means run forever.
    if (argc >= 2) {
        g_max_iterations = atoi(argv[1]);
        if (g_max_iterations < 0) {
            g_max_iterations = 0;
        }
    }

    srand((unsigned int)time(NULL));
    app_main();
    return 0;
}
