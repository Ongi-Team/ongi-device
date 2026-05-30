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
static bool s_refresh_pending = false;
static uint32_t s_refresh_request_count = 0;

static const SlotEntry s_fixture_slots[] = {
    { .slot_id = 1, .hour = 7, .minute = 0, .triggered = false },
    { .slot_id = 2, .hour = 8, .minute = 30, .triggered = false },
    { .slot_id = 3, .hour = 10, .minute = 0, .triggered = false },
    { .slot_id = 4, .hour = 20, .minute = 5, .triggered = false },
    { .slot_id = 5, .hour = 20, .minute = 10, .triggered = false },
    { .slot_id = 6, .hour = 20, .minute = 30, .triggered = false },
    { .slot_id = 7, .hour = 21, .minute = 0, .triggered = false },
    { .slot_id = 8, .hour = 22, .minute = 30, .triggered = false },
};

static void log_applied_slots(const SlotEntry *slots, size_t count, uint32_t version)
{
    for (size_t i = 0; i < count; i++) {
        ESP_LOGI(TAG, "active slot: version=%u slot_id=%u time=%02u:%02u",
                 (unsigned int)version,
                 (unsigned int)slots[i].slot_id,
                 (unsigned int)slots[i].hour,
                 (unsigned int)slots[i].minute);
    }
}

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
    s_refresh_pending = false;
    s_refresh_request_count = 0;
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
    log_applied_slots(slots, count, s_version);
    return ESP_OK;
}

// Apply a predefined fixture schedule for testing purposes
esp_err_t schedule_store_apply_fixture(void)
{
    ESP_LOGI(TAG, "Applying fixture schedule data");
    return schedule_store_apply(s_fixture_slots, sizeof(s_fixture_slots) / sizeof(s_fixture_slots[0]));
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

// SCHEDULE_UPDATED from MQTT
esp_err_t schedule_store_request_refresh(void)
{
    if (s_schedule_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // Acquire the mutex to safely update the refresh request state
    if (xSemaphoreTake(s_schedule_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Critical section: set the refresh pending flag and increment the request count
    s_refresh_pending = true;
    s_refresh_request_count++;
    uint32_t request_count = s_refresh_request_count;

    // Release the mutex after updating the refresh request state
    xSemaphoreGive(s_schedule_mutex);

    ESP_LOGI(TAG, "Schedule refresh requested: request_count=%u", (unsigned int)request_count);
    return ESP_OK;
}

esp_err_t schedule_store_consume_refresh_request(bool *was_pending, uint32_t *request_count)
{
    if (was_pending == NULL || request_count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_schedule_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // Acquire the mutex to safely read and update the refresh request state
    if (xSemaphoreTake(s_schedule_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    // Critical section: return the current refresh pending state and request count, then reset the pending flag
    *was_pending = s_refresh_pending;
    *request_count = s_refresh_request_count;
    s_refresh_pending = false;

    // Release the mutex after reading and updating the refresh request state
    xSemaphoreGive(s_schedule_mutex);

    return ESP_OK;
}

// Clear all schedule slots from the store
esp_err_t schedule_store_clear(void)
{
    return schedule_store_apply(NULL, 0);
}
