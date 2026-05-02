#include <stdio.h>
#include "wifi_heartbeat.h"
#include "storage_nvs.h"
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
}
