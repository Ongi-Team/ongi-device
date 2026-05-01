#include <stdio.h>
#include "wifi_heartbeat.h"
#include "storage_nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    nvs_init();
    wifi_init();
    xTaskCreate(
        heartbeat_task, 
        "heartbeat_task", 
        HEARTBEAT_TASK_STACK_SIZE, 
        NULL, 
        HEARTBEAT_TASK_PRIORITY, 
        NULL
    );
}
