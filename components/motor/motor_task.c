#include "motor_task.h"
#include "dispense.h"
#include "event_queue.h"
#include "servo_driver_factory.h"
#include "intake_detector_factory.h"
#include "esp_log.h"
#include "esp_timer.h"
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

    const IntakeDetector *detector = get_default_intake_detector();
    if (detector == NULL) {
        ESP_LOGE(TAG, "intake detector is not initialized");
        vTaskDelete(NULL);
        return;
    }

    const ServoDriver *servo = get_default_servo_driver();
    if (servo == NULL || servo->open == NULL || servo->close == NULL) {
        ESP_LOGE(TAG, "servo driver is not initialized");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        DispenseEvent event;

        if (xQueueReceive(dispense_queue, &event, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "Processing dispense event for slot ID: %d", event.slot_id);

            // Open the servo to dispense medication
            servo->open(event.slot_id);

            // Wait for intake detection with a timeout
            IntakeResult result = detector->wait(event.slot_id, pdMS_TO_TICKS(3000)); // 3-second timeout for testing
            
            bool is_event_pushed = false;
            MedicationStatus status;

            switch (result) {
                case INTAKE_DETECTED:
                    ESP_LOGI(TAG, "Intake detected for slot ID: %d", event.slot_id);
                    is_event_pushed = true;
                    status = MEDICATION_TAKEN;
                    break;
                case INTAKE_TIMEOUT:
                    ESP_LOGI(TAG, "Intake timeout for slot ID: %d", event.slot_id);
                    is_event_pushed = true;
                    status = MEDICATION_MISSED;
                    break;
                default:
                    ESP_LOGW(TAG, "Unknown intake result for slot ID: %d", event.slot_id);
                    break;
            }

            if (is_event_pushed) {
                MedicationEvent medication_event = {
                    .slot_id = event.slot_id,
                    .status = status,
                    .timestamp = esp_timer_get_time() / 1000 // Convert microseconds to milliseconds
                };
                
                esp_err_t err = event_queue_push(&medication_event);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to push %s event for slot ID: %d", 
                        status == MEDICATION_TAKEN ? "TAKEN" : "MISSED", event.slot_id);
                }
            }

            servo->close(event.slot_id);
        }
    }
}