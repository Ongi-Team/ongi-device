#include "event_sender.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "event_sender";

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

esp_err_t event_sender_send(const MedicationEvent *event) {
    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char json_payload[64];
    esp_err_t err = build_medication_event_payload(event, json_payload, sizeof(json_payload));
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "Medication event payload ready: slot=%u status=%s",
             (unsigned int)event->slot_id,
             medication_status_to_string(event->status));

    return ESP_OK;
}
