#include "intake.h"
#include "ads1115.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <stddef.h>

static const char *TAG = "intake";

#define INTAKE_I2C_SDA_GPIO GPIO_NUM_21
#define INTAKE_I2C_SCL_GPIO GPIO_NUM_22

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

    return ESP_OK;
}
