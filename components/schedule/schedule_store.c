#include "schedule_store.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "schedule_store";

static SemaphoreHandle_t s_schedule_mutex = NULL;
static SlotEntry s_slots[SCHEDULE_SLOT_COUNT];
static size_t s_slot_count = 0;
static uint32_t s_version = 0;

static esp_err_t validate_slots(const SlotEntry *slots, size_t count)
{
    if (count > SCHEDULE_SLOT_COUNT) {
        return ESP_ERR_INVALID_SIZE;
    }

    if (count > 0 && slots == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    for (size_t i = 0; i < count; i++) {
        if (slots[i].slot_id == 0 || slots[i].slot_id > SCHEDULE_SLOT_COUNT ||
            slots[i].hour > 23 || slots[i].minute > 59) {
            return ESP_ERR_INVALID_ARG;
        }
    }

    return ESP_OK;
}

esp_err_t schedule_store_init(void)
{
    if (s_schedule_mutex != NULL) {
        return ESP_OK;
    }

    // Create a mutex for synchronizing access to the schedule store
    s_schedule_mutex = xSemaphoreCreateMutex();
    if (s_schedule_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create schedule store mutex");
        return ESP_ERR_NO_MEM;
    }

    // Initialize the schedule store with empty data
    s_slot_count = 0;
    s_version = 0;
    memset(s_slots, 0, sizeof(s_slots));

    ESP_LOGI(TAG, "Schedule store initialized");
    return ESP_OK;
}

// Replace the entire schedule store data with the provided slots and count
esp_err_t schedule_store_apply(const SlotEntry *slots, size_t count)
{
    esp_err_t err = validate_slots(slots, count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Invalid schedule data: err=%s count=%u",
                 esp_err_to_name(err),
                 (unsigned int)count);
        return err;
    }

    if (s_schedule_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // Acquire the mutex to safely update the schedule store data
    if (xSemaphoreTake(s_schedule_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Critical section: update the schedule store data
    memset(s_slots, 0, sizeof(s_slots));
    if (count > 0) {
        memcpy(s_slots, slots, count * sizeof(SlotEntry));
    }
    s_slot_count = count;
    s_version++;

    // Release the mutex after updating the data
    xSemaphoreGive(s_schedule_mutex);

    ESP_LOGI(TAG, "Applied schedule snapshot: count=%u version=%u",
             (unsigned int)count,
             (unsigned int)s_version);
    return ESP_OK;
}

// Get a snapshot of the current schedule store data
esp_err_t schedule_store_get_snapshot(ScheduleSnapshot *snapshot)
{
    if (snapshot == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_schedule_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // Acquire the mutex to safely read the schedule store data
    if (xSemaphoreTake(s_schedule_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Critical section: copy the current schedule store data into the snapshot
    memset(snapshot, 0, sizeof(*snapshot));
    memcpy(snapshot->slots, s_slots, s_slot_count * sizeof(SlotEntry));
    snapshot->count = s_slot_count;
    snapshot->version = s_version;

    // Release the mutex after reading the data
    xSemaphoreGive(s_schedule_mutex);

    return ESP_OK;
}

// Clear all schedule slots from the store
esp_err_t schedule_store_clear(void)
{
    return schedule_store_apply(NULL, 0);
}
