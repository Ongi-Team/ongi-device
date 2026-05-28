#include "event_queue.h"
#include "event_sender.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "event_queue";
static QueueHandle_t s_event_queue = NULL;
static TaskHandle_t s_event_queue_task = NULL;

static void log_event_queue_stack_high_water(const char *phase);
static esp_err_t requeue_transient_event(const MedicationEvent *event);
static esp_err_t send_event_with_retry(const MedicationEvent *event);
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

static void log_event_queue_stack_high_water(const char *phase) {
    UBaseType_t high_water_words = uxTaskGetStackHighWaterMark(NULL);
    ESP_LOGI(TAG, "event_queue_task stack high water: phase=%s free_words=%u",
             phase,
             (unsigned int)high_water_words);
}

static esp_err_t requeue_transient_event(const MedicationEvent *event) {
    if (s_event_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xQueueSend(s_event_queue, event, pdMS_TO_TICKS(EVENT_QUEUE_SEND_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGW(TAG, "Requeued transient medication event: slot_id=%d", event->slot_id);
    return ESP_OK;
}

static esp_err_t send_event_with_retry(const MedicationEvent *event) {
    const int max_attempts = 3;
    const TickType_t retry_delay_ticks = pdMS_TO_TICKS(2000);
    esp_err_t last_err = ESP_FAIL;

    for (int attempt = 1; attempt <= max_attempts; attempt++) {
        esp_err_t err = event_sender_send(event);
        if (err == ESP_OK) {
            if (attempt > 1) {
                ESP_LOGI(TAG, "Medication event sent after retry: slot_id=%d attempt=%d",
                         event->slot_id,
                         attempt);
            }
            return ESP_OK;
        }
        last_err = err;

        if (!event_sender_is_transient_error(err)) {
            ESP_LOGE(TAG, "Medication event send failed permanently: slot_id=%d err=%s",
                     event->slot_id,
                     esp_err_to_name(err));
            return err;
        }

        ESP_LOGW(TAG, "Medication event send failed: slot_id=%d attempt=%d/%d err=%s",
                 event->slot_id,
                 attempt,
                 max_attempts,
                 esp_err_to_name(err));

        if (attempt < max_attempts) {
            vTaskDelay(retry_delay_ticks);
        }
    }

    return last_err;
}

static void event_queue_task(void *arg) {
    MedicationEvent event;

    log_event_queue_stack_high_water("start");

    while (1) {
        if (xQueueReceive(s_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            esp_err_t err = send_event_with_retry(&event);
            if (err != ESP_OK) {
                if (event_sender_is_transient_error(err)) {
                    esp_err_t requeue_err = requeue_transient_event(&event);
                    if (requeue_err != ESP_OK) {
                        ESP_LOGE(TAG, "Failed to preserve transient event: slot_id=%d err=%s",
                                 event.slot_id,
                                 esp_err_to_name(requeue_err));
                    }
                    continue;
                }

                ESP_LOGE(TAG, "Failed to send event: slot_id=%d", event.slot_id);
            }
            log_event_queue_stack_high_water("after_send");
        }
    }
}

QueueHandle_t get_event_queue(void) {
    return s_event_queue;
}
