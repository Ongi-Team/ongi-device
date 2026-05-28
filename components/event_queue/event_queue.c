#include "event_queue.h"
#include "esp_log.h"

static const char *TAG = "event_queue";
static QueueHandle_t s_event_queue = NULL;

esp_err_t event_queue_init(void) {
    // Check if the queue is already initialized
    if (s_event_queue != NULL) {
        ESP_LOGW(TAG, "Event queue is already initialized");
        return ESP_OK;
    }

    s_event_queue = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(MedicationEvent));

    if (s_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Event queue initialized");
    return ESP_OK;
}

esp_err_t event_queue_push(const MedicationEvent *event) {
    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_event_queue == NULL) {
        ESP_LOGE(TAG, "Event queue not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (xQueueSend(s_event_queue, event, 0) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to push event");
        return ESP_FAIL;
    }

    return ESP_OK;
}

QueueHandle_t get_event_queue(void) {
    return s_event_queue;
}
