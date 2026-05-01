#include <stdio.h>
#include "wifi_heartbeat.h"
#include "storage_nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    nvs_init();
    wifi_init();
    xTaskCreate(heartbeat_task, "heartbeat_task", 4096, NULL, 5, NULL);
}
