#ifndef ONGI_MQTT_CLIENT_H
#define ONGI_MQTT_CLIENT_H

#include "esp_err.h"

#define ONGI_MQTT_TASK_STACK_SIZE 4096
#define ONGI_MQTT_TASK_PRIORITY 5

esp_err_t ongi_mqtt_client_init(void);
esp_err_t ongi_mqtt_client_start(void);
void ongi_mqtt_client_task(void *arg);

#endif // ONGI_MQTT_CLIENT_H
