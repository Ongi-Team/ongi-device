#ifndef ONGI_MQTT_CLIENT_H
#define ONGI_MQTT_CLIENT_H

#include "esp_err.h"

esp_err_t ongi_mqtt_client_init(void);
esp_err_t ongi_mqtt_client_start(void);

#endif // ONGI_MQTT_CLIENT_H
