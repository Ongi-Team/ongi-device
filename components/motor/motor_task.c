#include "motor_task.h"
#include "dispense.h"
#include "event_queue.h"
#include "servo_driver_factory.h"
#include "intake_detector_factory.h"
#include "slot_map.h"
#include "esp_log.h"
#include <sys/time.h>
#include <time.h>
#include "freertos/task.h"

static const char *TAG = "motor";
static QueueHandle_t s_motor_command_queue = NULL;
static QueueSetHandle_t s_motor_queue_set = NULL;
static bool s_motor_queues_registered = false;

static int64_t get_timestamp_ms(void);
static bool is_time_synced(void);
static const char *motor_command_type_name(MotorCommandType type);
static bool is_valid_motor_command(MotorCommand command);
static esp_err_t process_dispense_event(DispenseEvent event, const ServoDriver *servo, const IntakeDetector *detector);
static esp_err_t process_motor_command(MotorCommand command, const ServoDriver *servo);
static esp_err_t run_all_slot_command(MotorCommand command, const ServoDriver *servo);
static bool tick_deadline_expired(TickType_t now, TickType_t deadline);
static TickType_t get_auto_close_wait_ticks(TickType_t deadline);

esp_err_t motor_init(void) {
    QueueHandle_t dispense_queue = get_dispense_queue();
    if (dispense_queue == NULL) {
        ESP_LOGE(TAG, "dispense queue is not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (s_motor_command_queue == NULL) {
        s_motor_command_queue = xQueueCreate(MOTOR_COMMAND_QUEUE_LENGTH, sizeof(MotorCommand));
        if (s_motor_command_queue == NULL) {
            ESP_LOGE(TAG, "motor command queue init failed");
            return ESP_ERR_NO_MEM;
        }
    }

    if (s_motor_queue_set == NULL) {
        s_motor_queue_set = xQueueCreateSet(DISPENSE_QUEUE_LENGTH + MOTOR_COMMAND_QUEUE_LENGTH);
        if (s_motor_queue_set == NULL) {
            ESP_LOGE(TAG, "motor queue set init failed");
            return ESP_ERR_NO_MEM;
        }
    }

    if (!s_motor_queues_registered) {
        if (xQueueAddToSet(dispense_queue, s_motor_queue_set) != pdPASS ||
            xQueueAddToSet(s_motor_command_queue, s_motor_queue_set) != pdPASS) {
            ESP_LOGE(TAG, "failed to add motor queues to queue set");
            return ESP_FAIL;
        }
        s_motor_queues_registered = true;
    }

    const ServoDriver *servo = get_default_servo_driver();
    if (servo == NULL) {
        ESP_LOGE(TAG, "servo driver init failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t motor_command_enqueue(MotorCommand command) {
    if (s_motor_command_queue == NULL) {
        ESP_LOGE(TAG, "motor command queue is not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (!is_valid_motor_command(command)) {
        ESP_LOGW(TAG, "invalid motor command rejected");
        return ESP_ERR_INVALID_ARG;
    }

    if (xQueueSend(s_motor_command_queue, &command, pdMS_TO_TICKS(MOTOR_COMMAND_SEND_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGE(TAG, "motor command enqueue failed: command=%s", motor_command_type_name(command.type));
        return ESP_ERR_TIMEOUT;
    }

    ESP_LOGI(TAG, "motor command enqueued: command=%s", motor_command_type_name(command.type));
    return ESP_OK;
}

QueueHandle_t get_motor_command_queue(void) {
    return s_motor_command_queue;
}

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

    if (s_motor_command_queue == NULL) {
        ESP_LOGE(TAG, "motor command queue is not initialized");
        vTaskDelete(NULL);
        return;
    }

    if (s_motor_queue_set == NULL || !s_motor_queues_registered) {
        ESP_LOGE(TAG, "motor queue set is not initialized");
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

    bool auto_close_pending = false;
    TickType_t auto_close_deadline = 0;

    while (1) {
        TickType_t wait_ticks = auto_close_pending ? get_auto_close_wait_ticks(auto_close_deadline) : portMAX_DELAY;
        QueueSetMemberHandle_t ready_queue = xQueueSelectFromSet(s_motor_queue_set, wait_ticks);

        if (ready_queue == NULL) {
            if (auto_close_pending && tick_deadline_expired(xTaskGetTickCount(), auto_close_deadline)) {
                MotorCommand auto_close_command = { .type = MOTOR_COMMAND_CLOSE_ALL };
                ESP_LOGW(TAG, "OPEN_ALL auto-close timeout reached; closing all slots");
                (void)process_motor_command(auto_close_command, servo);
                auto_close_pending = false;
            }
            continue;
        }

        if (ready_queue == dispense_queue) {
            DispenseEvent event;
            if (xQueueReceive(dispense_queue, &event, 0) == pdTRUE) {
                (void)process_dispense_event(event, servo, detector);
            }
        } else if (ready_queue == s_motor_command_queue) {
            MotorCommand command;
            if (xQueueReceive(s_motor_command_queue, &command, 0) == pdTRUE) {
                (void)process_motor_command(command, servo);
                if (command.type == MOTOR_COMMAND_OPEN_ALL) {
                    auto_close_deadline = xTaskGetTickCount() + pdMS_TO_TICKS(MOTOR_OPEN_ALL_AUTO_CLOSE_TIMEOUT_MS);
                    auto_close_pending = true;
                    ESP_LOGI(TAG, "OPEN_ALL auto-close scheduled: timeout_ms=%d",
                             MOTOR_OPEN_ALL_AUTO_CLOSE_TIMEOUT_MS);
                } else if (command.type == MOTOR_COMMAND_CLOSE_ALL) {
                    auto_close_pending = false;
                    ESP_LOGI(TAG, "OPEN_ALL auto-close cleared by CLOSE_ALL");
                }
            }
        }
    }
}

static const char *motor_command_type_name(MotorCommandType type) {
    switch (type) {
        case MOTOR_COMMAND_OPEN_ALL:
            return "OPEN_ALL";
        case MOTOR_COMMAND_CLOSE_ALL:
            return "CLOSE_ALL";
        default:
            return "UNKNOWN";
    }
}

static bool is_valid_motor_command(MotorCommand command) {
    return command.type == MOTOR_COMMAND_OPEN_ALL ||
           command.type == MOTOR_COMMAND_CLOSE_ALL;
}

static esp_err_t process_dispense_event(DispenseEvent event, const ServoDriver *servo, const IntakeDetector *detector) {
    ESP_LOGI(TAG, "Processing dispense event for slot ID: %d", event.slot_id);

    esp_err_t open_err = servo->open(event.slot_id);
    if (open_err != ESP_OK) {
        ESP_LOGE(TAG, "servo open failed for slot %d: %s", event.slot_id, esp_err_to_name(open_err));
        // TODO: open failure means dispense never started — add a device fault event type to report to server
        return open_err;
    }

    // servo->open blocks until fade completes; give the user time to take medication
    IntakeResult result = detector->wait(event.slot_id, pdMS_TO_TICKS(15000));

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

    esp_err_t close_err = servo->close(event.slot_id);
    if (close_err != ESP_OK) {
        ESP_LOGE(TAG, "servo close failed for slot %d: %s", event.slot_id, esp_err_to_name(close_err));
        // TODO: if close fails while another slot dispenses, medication mixing is possible — response strategy TBD
    }

    if (is_event_pushed) {
        MedicationEvent medication_event = {
            .slot_id = event.slot_id,
            .status = status,
            .timestamp = is_time_synced() ? get_timestamp_ms() : 0
        };

        esp_err_t err = event_queue_push(&medication_event);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to push %s event for slot ID: %d",
                status == MEDICATION_TAKEN ? "TAKEN" : "MISSED", event.slot_id);
        } else {
            ESP_LOGI(TAG, "Pushed %s event for slot ID: %d to event queue",
                status == MEDICATION_TAKEN ? "TAKEN" : "MISSED", event.slot_id);
        }
    }

    return close_err;
}

static esp_err_t process_motor_command(MotorCommand command, const ServoDriver *servo) {
    if (!is_valid_motor_command(command)) {
        ESP_LOGW(TAG, "invalid motor command ignored");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Processing motor command: command=%s", motor_command_type_name(command.type));
    esp_err_t err = run_all_slot_command(command, servo);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "motor command failed: command=%s err=%s",
                 motor_command_type_name(command.type),
                 esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "motor command processed: command=%s", motor_command_type_name(command.type));
    return ESP_OK;
}

static esp_err_t run_all_slot_command(MotorCommand command, const ServoDriver *servo) {
    esp_err_t first_close_err = ESP_OK;

    for (uint8_t slot_id = 1; slot_id <= SLOT_COUNT; slot_id++) {
        const SlotHwConfig *slot_config = NULL;
        esp_err_t err = slot_map_get(slot_id, &slot_config);
        if (err != ESP_OK || slot_config == NULL) {
            ESP_LOGE(TAG, "slot validation failed: slot=%d err=%s", slot_id, esp_err_to_name(err));
            return err == ESP_OK ? ESP_ERR_INVALID_STATE : err;
        }

        if (command.type == MOTOR_COMMAND_OPEN_ALL) {
            err = servo->open(slot_id);
        } else {
            err = servo->close(slot_id);
        }

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "all-slot command step failed: command=%s slot=%d err=%s",
                     motor_command_type_name(command.type),
                     slot_id,
                     esp_err_to_name(err));
            if (command.type == MOTOR_COMMAND_CLOSE_ALL) {
                if (first_close_err == ESP_OK) {
                    first_close_err = err;
                }
                continue;
            }
            return err;
        }
    }

    return first_close_err;
}

static bool tick_deadline_expired(TickType_t now, TickType_t deadline) {
    return (int32_t)(now - deadline) >= 0;
}

static TickType_t get_auto_close_wait_ticks(TickType_t deadline) {
    TickType_t now = xTaskGetTickCount();
    if (tick_deadline_expired(now, deadline)) {
        return 0;
    }

    return deadline - now;
}

static bool is_time_synced(void) {
    time_t now;
    time(&now);

    return now > 1704067200; // Check if current time is after Jan 1, 2024
}
static int64_t get_timestamp_ms(void) {
    struct timeval now;
    gettimeofday(&now, NULL);

    return (int64_t)now.tv_sec * 1000 + now.tv_usec / 1000;
}
