#include "intake.h"
#include "ads1115.h"
#include "slot_map.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <stddef.h>

static const char *TAG = "intake";

#define INTAKE_I2C_SDA_GPIO GPIO_NUM_21
#define INTAKE_I2C_SCL_GPIO GPIO_NUM_22
#define INTAKE_DEFAULT_RAW_THRESHOLD 1000

static SemaphoreHandle_t s_threshold_mutex = NULL;
static int16_t s_thresholds[SLOT_COUNT];

static esp_err_t validate_slot_id(uint8_t slot_id)
{
    if (slot_id < 1 || slot_id > SLOT_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

static esp_err_t init_thresholds(void)
{
    if (s_threshold_mutex == NULL) {
        s_threshold_mutex = xSemaphoreCreateMutex();
        if (s_threshold_mutex == NULL) {
            ESP_LOGE(TAG, "threshold mutex create failed");
            return ESP_ERR_NO_MEM;
        }

        for (uint8_t i = 0; i < SLOT_COUNT; i++) {
            s_thresholds[i] = INTAKE_DEFAULT_RAW_THRESHOLD;
        }
    }

    return ESP_OK;
}

esp_err_t intake_get_slot_threshold(uint8_t slot_id, int16_t *out_threshold)
{
    if (out_threshold == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = validate_slot_id(slot_id);
    if (err != ESP_OK) {
        return err;
    }

    if (s_threshold_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(s_threshold_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }

    *out_threshold = s_thresholds[slot_id - 1];
    xSemaphoreGive(s_threshold_mutex);
    return ESP_OK;
}

esp_err_t intake_set_slot_threshold(uint8_t slot_id, int16_t threshold)
{
    esp_err_t err = validate_slot_id(slot_id);
    if (err != ESP_OK) {
        return err;
    }

    if (s_threshold_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(s_threshold_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }

    s_thresholds[slot_id - 1] = threshold;
    xSemaphoreGive(s_threshold_mutex);
    ESP_LOGI(TAG, "intake threshold set: slot=%u threshold=%d", slot_id, threshold);
    return ESP_OK;
}

esp_err_t intake_read_slot_raw(uint8_t slot_id, int16_t *out_raw)
{
    if (out_raw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const SlotHwConfig *cfg = NULL;
    esp_err_t err = slot_map_get(slot_id, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "slot map lookup failed: slot=%u err=%s", slot_id, esp_err_to_name(err));
        return err;
    }

    err = ads1115_read_channel(cfg->ads1115_addr, cfg->ads1115_channel, out_raw);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADS1115 read failed: slot=%u addr=0x%02x ch=%u err=%s",
                 slot_id, cfg->ads1115_addr, cfg->ads1115_channel, esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

static esp_err_t log_slot_readings(void)
{
    for (uint8_t slot_id = 1; slot_id <= SLOT_COUNT; slot_id++) {
        int16_t raw = 0;
        esp_err_t err = intake_read_slot_raw(slot_id, &raw);
        if (err != ESP_OK) {
            return err;
        }

        ESP_LOGI(TAG, "intake sensor raw: slot=%u value=%d", slot_id, raw);
    }

    return ESP_OK;
}

esp_err_t intake_init(void)
{
    esp_err_t err = init_thresholds();
    if (err != ESP_OK) {
        return err;
    }

    err = ads1115_bus_init(INTAKE_I2C_SDA_GPIO, INTAKE_I2C_SCL_GPIO);
    if (err != ESP_OK) {
#ifdef CI_BUILD
        ESP_LOGW(TAG, "ADS1115 bus init failed, continuing without sensor raw reads: %s", esp_err_to_name(err));
        return ESP_OK;
#else
        ESP_LOGE(TAG, "ADS1115 bus init failed: %s", esp_err_to_name(err));
        return err;
#endif
    }

    uint8_t addrs[] = {ADS1115_ADDR_LO, ADS1115_ADDR_HI};
    for (size_t i = 0; i < sizeof(addrs) / sizeof(addrs[0]); i++) {
        bool found = false;

        err = ads1115_detect(addrs[i], &found);
        if (err != ESP_OK) {
#ifdef CI_BUILD
            ESP_LOGW(TAG, "ADS1115 detect failed, continuing without sensor raw reads: addr=0x%02x err=%s",
                     addrs[i], esp_err_to_name(err));
            return ESP_OK;
#else
            ESP_LOGE(TAG, "ADS1115 detect failed: addr=0x%02x err=%s", addrs[i], esp_err_to_name(err));
            return err;
#endif
        }

        if (!found) {
#ifdef CI_BUILD
            ESP_LOGW(TAG, "ADS1115 not found, continuing without sensor raw reads: addr=0x%02x", addrs[i]);
            return ESP_OK;
#else
            ESP_LOGE(TAG, "ADS1115 not found: addr=0x%02x", addrs[i]);
            return ESP_ERR_NOT_FOUND;
#endif
        }
    }

    err = log_slot_readings();
    if (err != ESP_OK) {
#ifdef CI_BUILD
        ESP_LOGW(TAG, "ADS1115 initial raw logging failed, continuing without sensor raw reads: %s",
                 esp_err_to_name(err));
        return ESP_OK;
#else
        return err;
#endif
    }

    return ESP_OK;
}
