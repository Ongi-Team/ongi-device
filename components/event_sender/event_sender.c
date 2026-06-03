#include "event_sender.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#if defined(CI_BUILD)
    #include "http_config_example.h"
    #include "wifi_config_example.h"
#else
    #include "http_config.h"
    #include "wifi_config.h"
#endif

static const char *TAG = "event_sender";

static bool is_retryable_http_status(int status_code);
static esp_err_t prepare_medication_event_request(esp_http_client_handle_t client, const char *json_payload);

bool event_sender_is_transient_error(esp_err_t err) {
    return err == ESP_ERR_TIMEOUT ||
           err == ESP_ERR_HTTP_CONNECT ||
           err == ESP_ERR_HTTP_WRITE_DATA ||
           err == ESP_ERR_HTTP_FETCH_HEADER ||
           err == ESP_ERR_HTTP_CONNECTING ||
           err == ESP_ERR_HTTP_EAGAIN ||
           err == ESP_ERR_HTTP_CONNECTION_CLOSED ||
           err == ESP_ERR_HTTP_READ_TIMEOUT ||
           err == ESP_ERR_HTTP_INCOMPLETE_DATA;
}

static const char *medication_status_to_string(MedicationStatus status) {
    switch (status) {
        case MEDICATION_TAKEN:
            return "TAKEN";
        case MEDICATION_MISSED:
            return "MISSED";
        default:
            return NULL;
    }
}

static bool is_retryable_http_status(int status_code) {
    return status_code == 408 || status_code == 429 || status_code >= 500;
}

static esp_err_t build_medication_event_payload(const MedicationEvent *event, char *json_payload, size_t json_payload_size) {
    if (event == NULL || json_payload == NULL || json_payload_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const char *status = medication_status_to_string(event->status);
    if (status == NULL) {
        ESP_LOGE(TAG, "Invalid medication status: %d", event->status);
        return ESP_ERR_INVALID_ARG;
    }

    int written = snprintf(
        json_payload,
        json_payload_size,
        "{\"slotNumber\":%u,\"status\":\"%s\"}",
        (unsigned int)event->slot_id,
        status
    );
    if (written < 0 || (size_t)written >= json_payload_size) {
        ESP_LOGE(TAG, "Failed to build medication event payload");
        return ESP_ERR_INVALID_SIZE;
    }

    return ESP_OK;
}

static esp_err_t prepare_medication_event_request(esp_http_client_handle_t client, const char *json_payload) {
    if (client == NULL || json_payload == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = esp_http_client_set_header(client, "Content-Type", "application/json");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Content-Type header: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_http_client_set_header(client, "Device-Token", CONFIG_DEVICE_TOKEN);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Device-Token header: %s", esp_err_to_name(err));
        return err;
    }

    err = esp_http_client_set_post_field(client, json_payload, strlen(json_payload));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set medication event POST body: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t event_sender_send(const MedicationEvent *event) {
    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char json_payload[64];
    esp_err_t err = build_medication_event_payload(event, json_payload, sizeof(json_payload));
    if (err != ESP_OK) {
        return err;
    }

    esp_http_client_config_t config = {
        .url = MEDICATION_EVENT_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize medication event HTTP client");
        return ESP_FAIL;
    }

    err = prepare_medication_event_request(client, json_payload);
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        return err;
    }

    ESP_LOGI(TAG, "Sending medication event: slot=%u status=%s",
             (unsigned int)event->slot_id,
             medication_status_to_string(event->status));

    err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send medication event: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    int status_code = esp_http_client_get_status_code(client);
    if (status_code < 200 || status_code >= 300) {
        ESP_LOGE(TAG, "Medication event POST failed: http_status=%d slot=%u status=%s",
                 status_code,
                 (unsigned int)event->slot_id,
                 medication_status_to_string(event->status));
        esp_http_client_cleanup(client);
        return is_retryable_http_status(status_code) ? ESP_ERR_TIMEOUT : ESP_ERR_INVALID_RESPONSE;
    }

    ESP_LOGI(TAG, "Medication event POST completed: http_status=%d", status_code);

    esp_http_client_cleanup(client);
    return ESP_OK;
}
