#ifndef WIFI_HEARTBEAT_H
#define WIFI_HEARTBEAT_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#define HEARTBEAT_TASK_STACK_SIZE 8192
#define HEARTBEAT_TASK_PRIORITY 5
#define HEARTBEAT_INTERVAL_MS 60000

esp_err_t wifi_init();
esp_err_t wifi_wait_connected(TickType_t ticks_to_wait);
void heartbeat_task(void *pvParameters);

#endif // WIFI_HEARTBEAT_H
