#include "ongi_mqtt_client.h"

#include <stdbool.h>
#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"

#ifdef CI_BUILD
    #include "wifi_config_example.h"
    #include "mqtt_config_example.h"
#else
    #include "wifi_config.h"
    #include "mqtt_config.h"
#endif

static const char *TAG = "ongi-mqtt-client";

#define MQTT_TOPIC_BUFFER_SIZE 160

typedef struct {
    char open_all[MQTT_TOPIC_BUFFER_SIZE];
    char close_all[MQTT_TOPIC_BUFFER_SIZE];
    char schedule_updated[MQTT_TOPIC_BUFFER_SIZE];
    bool initialized;
} OngiMqttTopics;

static OngiMqttTopics s_command_topics;

static esp_err_t build_command_topic(char *buffer, size_t buffer_size, const char *command) {
    if (buffer == NULL || buffer_size == 0 || command == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (CONFIG_DEVICE_TOKEN[0] == '\0') {
        ESP_LOGE(TAG, "Device token is empty; cannot build MQTT command topic");
        return ESP_ERR_INVALID_ARG;
    }

    int written = snprintf(
        buffer,
        buffer_size,
        "device/%s/command/%s",
        CONFIG_DEVICE_TOKEN,
        command
    );
    if (written < 0 || (size_t)written >= buffer_size) {
        ESP_LOGE(TAG, "MQTT command topic buffer is too small for command=%s", command);
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

static esp_err_t build_command_topics(void) {
    esp_err_t err = build_command_topic(
        s_command_topics.open_all,
        sizeof(s_command_topics.open_all),
        "open-all"
    );
    if (err != ESP_OK) {
        return err;
    }

    err = build_command_topic(
        s_command_topics.close_all,
        sizeof(s_command_topics.close_all),
        "close-all"
    );
    if (err != ESP_OK) {
        return err;
    }

    err = build_command_topic(
        s_command_topics.schedule_updated,
        sizeof(s_command_topics.schedule_updated),
        "schedule-updated"
    );
    if (err != ESP_OK) {
        return err;
    }

    s_command_topics.initialized = true;
    ESP_LOGI(TAG, "MQTT command topics prepared");
    return ESP_OK;
}

esp_err_t ongi_mqtt_client_init(void) {
    return build_command_topics();
}

esp_err_t ongi_mqtt_client_start(void) {
    return ESP_ERR_NOT_SUPPORTED;
}
