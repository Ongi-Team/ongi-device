#include <stdio.h>
#include "wifi_heartbeat.h"
#include "storage_nvs.h"
#include "rtc_driver.h"
#include "schedule.h"
#include "schedule_store.h"
#include "dispense.h"
#include "motor_task.h"
#include "event_queue.h"
#include "intake.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#if !defined(CI_BUILD) && !defined(TEST_BUILD)
#include "ongi_mqtt_client.h"
#endif

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

    // Initialize intake module and check for errors
    err = intake_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize intake");
        return;
    }

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

#if defined(CI_BUILD) || (defined(TEST_BUILD) && defined(DUMMY_TEST))
    // Temporary fixture schedule for CI/store integration and dummy test builds.
    err = schedule_store_apply_fixture();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to apply fixture schedule");
        return;
    }
#endif

#if !defined(CI_BUILD) && !(defined(TEST_BUILD) && defined(DUMMY_TEST))
    // Request the first server schedule fetch after RTC sync gates the schedule task.
    err = schedule_store_request_refresh();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to request initial schedule refresh");
        return;
    }
#endif

    // Initialize event queue and check for errors
    err = event_queue_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize event queue");
        return;
    }

    // Register motor queues before MQTT can receive remote commands
    err = motor_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize motor");
        return;
    }

    // Create the motor task
    BaseType_t motor_task_created = create_motor_task();

    if (motor_task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create motor task");
        return;
    }

#if !defined(CI_BUILD) && !defined(TEST_BUILD)
    // Initialize MQTT client and prepare command topics after motor queues are registered
    err = ongi_mqtt_client_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client: %s", esp_err_to_name(err));
        return;
    }

    // Create the MQTT client task after motor_task can consume remote commands
    BaseType_t mqtt_task_created = xTaskCreate(
        ongi_mqtt_client_task,
        "mqtt_client_task",
        ONGI_MQTT_TASK_STACK_SIZE,
        NULL,
        ONGI_MQTT_TASK_PRIORITY,
        NULL
    );
    if (mqtt_task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create MQTT client task");
        return;
    }
#endif

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
