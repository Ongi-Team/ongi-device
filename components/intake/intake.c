#include "intake.h"
#include "ads1115.h"
#include "slot_map.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <stddef.h>

static const char *TAG = "intake";

#define INTAKE_I2C_SDA_GPIO GPIO_NUM_21
#define INTAKE_I2C_SCL_GPIO GPIO_NUM_22

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
    esp_err_t err = ads1115_bus_init(INTAKE_I2C_SDA_GPIO, INTAKE_I2C_SCL_GPIO);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADS1115 bus init failed: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t addrs[] = {ADS1115_ADDR_LO, ADS1115_ADDR_HI};
    for (size_t i = 0; i < sizeof(addrs) / sizeof(addrs[0]); i++) {
        bool found = false;

        err = ads1115_detect(addrs[i], &found);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "ADS1115 detect failed: addr=0x%02x err=%s", addrs[i], esp_err_to_name(err));
            return err;
        }

        if (!found) {
            ESP_LOGE(TAG, "ADS1115 not found: addr=0x%02x", addrs[i]);
            return ESP_ERR_NOT_FOUND;
        }
    }

    return log_slot_readings();
}
