#include "event_queue.h"
#include "event_sender.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "event_queue";
static QueueHandle_t s_event_queue = NULL;
static TaskHandle_t s_event_queue_task = NULL;

static void event_queue_task(void *arg);

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

    BaseType_t task_created = xTaskCreate(
        event_queue_task,
        "event_queue_task",
        EVENTQUEUE_TASK_STACK_SIZE,
        NULL,
        EVENTQUEUE_TASK_PRIORITY,
        &s_event_queue_task
    );

    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create event queue task");
        vQueueDelete(s_event_queue);
        s_event_queue = NULL;
        return ESP_FAIL;
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

    if (xQueueSend(s_event_queue, event, pdMS_TO_TICKS(EVENT_QUEUE_SEND_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to push event");
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

static void event_queue_task(void *arg) {
    MedicationEvent event;

    while (1) {
        if (xQueueReceive(s_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            esp_err_t err = event_sender_send(&event);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Failed to send event: slot_id=%d", event.slot_id);
            }
        }
    }
}

QueueHandle_t get_event_queue(void) {
    return s_event_queue;
}
