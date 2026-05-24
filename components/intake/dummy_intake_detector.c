#include "dummy_intake_detector.h"
#include "freertos/task.h"
#include "esp_log.h"

const static char *TAG = "dummy_intake";

static esp_err_t dummy_intake_init(void) {
    return ESP_OK;
}

static IntakeResult dummy_intake_wait(uint8_t slot_id, TickType_t timeout_ticks) {
    ESP_LOGI(TAG, "dummy intake wait: slot_id=%d", slot_id);
    // Simulate waiting for an intake detection by delaying for the specified timeout
    vTaskDelay(timeout_ticks);
    return INTAKE_TIMEOUT;
}

const IntakeDetector DUMMY_INTAKE_DETECTOR = {
    .init = dummy_intake_init,
    .wait = dummy_intake_wait,
};