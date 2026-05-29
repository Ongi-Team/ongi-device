#include "ongi_mqtt_client.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "wifi_heartbeat.h"

#ifdef CI_BUILD
    #include "wifi_config_example.h"
    #include "mqtt_config_example.h"
#else
    #include "wifi_config.h"
    #include "mqtt_config.h"
#endif

static const char *TAG = "ongi-mqtt-client";

#define MQTT_TOPIC_BUFFER_SIZE 160
#define MQTT_START_RETRY_DELAY_MS 5000
#define MQTT_COMMAND_QOS 1

typedef struct {
    char open_all[MQTT_TOPIC_BUFFER_SIZE];
    char close_all[MQTT_TOPIC_BUFFER_SIZE];
    char schedule_updated[MQTT_TOPIC_BUFFER_SIZE];
    bool initialized;
} OngiMqttTopics;

static OngiMqttTopics s_command_topics;
static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_mqtt_started = false;

static bool mqtt_topic_matches(const char *event_topic, int event_topic_len, const char *expected_topic) {
    if (event_topic == NULL || event_topic_len < 0 || expected_topic == NULL) {
        return false;
    }

    size_t expected_len = strlen(expected_topic);
    return (size_t)event_topic_len == expected_len &&
           memcmp(event_topic, expected_topic, expected_len) == 0;
}

static const char *get_command_name_from_topic(const char *topic, int topic_len) {
    if (mqtt_topic_matches(topic, topic_len, s_command_topics.open_all)) {
        return "open-all";
    }

    if (mqtt_topic_matches(topic, topic_len, s_command_topics.close_all)) {
        return "close-all";
    }

    if (mqtt_topic_matches(topic, topic_len, s_command_topics.schedule_updated)) {
        return "schedule-updated";
    }

    return "unknown";
}

static void log_received_command_metadata(const esp_mqtt_event_handle_t event) {
    if (event == NULL) {
        ESP_LOGW(TAG, "MQTT data event missing event context");
        return;
    }

    const char *command = get_command_name_from_topic(event->topic, event->topic_len);
    ESP_LOGI(TAG,
             "MQTT command received: command=%s msg_id=%d topic_len=%d payload_len=%d total_payload_len=%d offset=%d qos=%d retain=%d dup=%d",
             command,
             event->msg_id,
             event->topic_len,
             event->data_len,
             event->total_data_len,
             event->current_data_offset,
             event->qos,
             event->retain,
             event->dup);
}

static esp_err_t subscribe_command_topic(esp_mqtt_client_handle_t client, const char *topic, const char *command) {
    if (client == NULL || topic == NULL || command == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int msg_id = esp_mqtt_client_subscribe(client, topic, MQTT_COMMAND_QOS);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to subscribe MQTT command topic: command=%s", command);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MQTT command subscribe requested: command=%s qos=%d msg_id=%d",
             command,
             MQTT_COMMAND_QOS,
             msg_id);
    return ESP_OK;
}

static esp_err_t subscribe_command_topics(esp_mqtt_client_handle_t client) {
    esp_err_t err = subscribe_command_topic(client, s_command_topics.open_all, "open-all");
    if (err != ESP_OK) {
        return err;
    }

    err = subscribe_command_topic(client, s_command_topics.close_all, "close-all");
    if (err != ESP_OK) {
        return err;
    }

    return subscribe_command_topic(client, s_command_topics.schedule_updated, "schedule-updated");
}

// MQTT event handler to log connection status and errors
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    (void)handler_args;
    (void)base;
    (void)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGI(TAG, "MQTT client connecting");
            break;
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT broker connected");
            if (subscribe_command_topics((esp_mqtt_client_handle_t)handler_args) != ESP_OK) {
                ESP_LOGE(TAG, "Failed to request MQTT command subscriptions");
            }
            break;
        case MQTT_EVENT_SUBSCRIBED: {
            esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
            ESP_LOGI(TAG, "MQTT command subscription acknowledged: msg_id=%d", event->msg_id);
            break;
        }
        case MQTT_EVENT_DATA:
            log_received_command_metadata((esp_mqtt_event_handle_t)event_data);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT broker disconnected");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGW(TAG, "MQTT client error event received");
            break;
        default:
            break;
    }
}

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
    if (!s_command_topics.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_mqtt_started) {
        return ESP_OK;
    }

    if (s_mqtt_client == NULL) {
        const esp_mqtt_client_config_t mqtt_cfg = {
            .broker.address.uri = MQTT_BROKER_URI,
            .credentials.client_id = MQTT_CLIENT_ID,
            .credentials.username = MQTT_USERNAME[0] == '\0' ? NULL : MQTT_USERNAME,
            .credentials.authentication.password = MQTT_PASSWORD[0] == '\0' ? NULL : MQTT_PASSWORD,
        };

        // Create MQTT client handle
        s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
        if (s_mqtt_client == NULL) {
            ESP_LOGE(TAG, "Failed to create MQTT client handle");
            return ESP_FAIL;
        }

        // Register MQTT event handler
        esp_err_t err = esp_mqtt_client_register_event(
            s_mqtt_client,
            ESP_EVENT_ANY_ID,
            mqtt_event_handler,
            s_mqtt_client
        );
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to register MQTT event handler: %s", esp_err_to_name(err));
            esp_mqtt_client_destroy(s_mqtt_client);
            s_mqtt_client = NULL;
            return err;
        }
    }

    // Start MQTT client
    esp_err_t err = esp_mqtt_client_start(s_mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
        return err;
    }

    s_mqtt_started = true;
    ESP_LOGI(TAG, "MQTT client start requested after Wi-Fi connection");
    return ESP_OK;
}

// MQTT client task to manage connection and retries
void ongi_mqtt_client_task(void *arg) {
    (void)arg;

    while (1) {
        // Wait for Wi-Fi connection before starting MQTT client
        esp_err_t err = wifi_wait_connected(portMAX_DELAY);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to wait for Wi-Fi connection: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(MQTT_START_RETRY_DELAY_MS));
            continue;
        }

        // Start MQTT client after Wi-Fi is connected
        err = ongi_mqtt_client_start();
        if (err == ESP_OK) {
            // MQTT client started successfully, exit the task
            vTaskDelete(NULL);
        }

        // If starting MQTT client failed, log the error and retry after a delay
        ESP_LOGW(TAG, "MQTT client start will retry: %s", esp_err_to_name(err));
        vTaskDelay(pdMS_TO_TICKS(MQTT_START_RETRY_DELAY_MS));
    }
}
