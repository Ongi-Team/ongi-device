#include "event_sender.h"
#include "esp_log.h"

static const char *TAG = "event_sender";

esp_err_t event_sender_send(const MedicationEvent *event) {
    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Medication event: slot=%d status=%s timestamp=%lld",
             event->slot_id,
             event->status == MEDICATION_TAKEN ? "TAKEN" : "MISSED",
             event->timestamp);

    return ESP_OK;
}
