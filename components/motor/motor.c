#include "motor.h"
#include "dispense.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "motor";

BaseType_t create_motor_task(void) {
    return xTaskCreate(
        motor_task,
        "motor_task",
        MOTOR_TASK_STACK_SIZE,
        NULL,
        MOTOR_TASK_PRIORITY,
        NULL
    );
}

void motor_task(void *arg) {
    QueueHandle_t dispense_queue = get_dispense_queue();

    if (dispense_queue == NULL) {
        ESP_LOGE(TAG, "dispense queue is not initialized");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        DispenseEvent event;

        if (xQueueReceive(dispense_queue, &event, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Processing dispense event for slot ID: %d", event.slot_id);
        }
    }
}