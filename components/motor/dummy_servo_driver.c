#include "servo_driver.h"
#include "esp_log.h"

const static char *TAG = "dummy_servo_driver";

static esp_err_t dummy_open(uint8_t slot_id) {
    ESP_LOGI(TAG, "dummy servo open: slot_id=%d", slot_id);
    return ESP_OK;
}

static esp_err_t dummy_close(uint8_t slot_id) {
    ESP_LOGI(TAG, "dummy servo close: slot_id=%d", slot_id);
    return ESP_OK;
}

const ServoDriver DUMMY_SERVO_DRIVER = {
    .open = dummy_open,
    .close = dummy_close,
};
