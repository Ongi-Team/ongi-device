#include <stdio.h>
#include "wifi_heartbeat.h"
#include "storage_nvs.h"
#include "rtc_driver.h"
#include "schedule.h"
#include "schedule_store.h"
#include "dispense.h"
#include "motor_task.h"
#include "event_queue.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

void app_main(void)
{
    esp_err_t err;
    static const char *TAG = "ongi-main";

    // Initialize NVS storage
    err = nvs_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS storage");
        return;
    }

    // Initialize WiFi and start heartbeat task
    err = wifi_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi");
        return;
    }

    // Initialize RTC and check for errors
    ESP_ERROR_CHECK(rtc_driver_init());

    // Create the heartbeat task
    BaseType_t heartbeat_task_created = xTaskCreate(
        heartbeat_task, 
        "heartbeat_task", 
        HEARTBEAT_TASK_STACK_SIZE, 
        NULL, 
        HEARTBEAT_TASK_PRIORITY, 
        NULL
    );

    if (heartbeat_task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create heartbeat task");
        return;
    }

    // Initialize dispense module and check for errors
    err = dispense_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize dispense");
        return;
    }

    // Initialize schedule store for API-provided schedules
    err = schedule_store_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize schedule store");
        return;
    }

#ifdef CI_BUILD
    // Temporary fixture schedule for CI/store integration testing only.
    err = schedule_store_apply_fixture();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to apply fixture schedule");
        return;
    }
#endif

    // Initialize event queue and check for errors
    err = event_queue_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize event queue");
        return;
    }
    
    // Create the motor task
    BaseType_t motor_task_created = create_motor_task();

    if (motor_task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create motor task");
        return;
    }

    // Create the schedule task
    BaseType_t schedule_task_created = xTaskCreate(
        schedule_task,
        "schedule_task",
        SCHEDULE_TASK_STACK_SIZE,
        NULL,
        SCHEDULE_TASK_PRIORITY,
        NULL
    );

    if (schedule_task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create schedule task");
        return; 
    }
}
