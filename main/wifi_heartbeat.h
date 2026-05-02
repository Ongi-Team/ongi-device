#ifndef WIFI_HEARTBEAT_H
#define WIFI_HEARTBEAT_H

#include "esp_err.h"

#define HEARTBEAT_TASK_STACK_SIZE 8192
#define HEARTBEAT_TASK_PRIORITY 5
#define HEARTBEAT_INTERVAL_MS 60000

esp_err_t wifi_init();
void heartbeat_task(void *pvParameters);

#endif // WIFI_HEARTBEAT_H