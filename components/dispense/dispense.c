#include "dispense.h"
#include "esp_log.h"

static const char *TAG = "dispense";
static QueueHandle_t s_dispense_queue = NULL;

esp_err_t dispense_init(void) {
    s_dispense_queue= xQueueCreate(DISPENSE_QUEUE_LENGTH, sizeof(DispenseEvent));

    if (s_dispense_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create dispense event queue");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Dispense module initialized successfully");
    return ESP_OK;
}

esp_err_t dispense_enqueue(uint8_t slot_id) {
    if (s_dispense_queue == NULL) {
        ESP_LOGE(TAG, "Dispense queue not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Create a dispense event with the specified slot ID
    DispenseEvent event = { .slot_id = slot_id };

    // Enqueue the dispense event without blocking; log an error if the queue is full
    if (xQueueSend(s_dispense_queue, &event, 0) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to enqueue dispense event");
        return ESP_FAIL;
    }

    return ESP_OK;
}

void dispense_task(void *arg) {
    DispenseEvent event;

    ESP_LOGI(TAG, "Dispense task started");

    while (1) {
        if (xQueueReceive(s_dispense_queue, &event, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Processing dispense event for slot ID: %d", event.slot_id);
        
            // TODO: servo motor control
        }
    }
}