#include "servo_motor.h"
#include "esp_log.h"

static const char *TAG = "servo_motor";

esp_err_t servo_motor_open(int slot_id) {
    ESP_LOGI(TAG, "servo open: slot_id=%d", slot_id);
    return ESP_OK;
}

esp_err_t servo_motor_close(int slot_id) {
    ESP_LOGI(TAG, "servo close: slot_id=%d", slot_id);
    return ESP_OK;
}