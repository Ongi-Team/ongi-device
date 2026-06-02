#include "schedule.h"
#include "schedule_fetcher.h"
#include "schedule_store.h"
#include "rtc_driver.h"
#include "dispense.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <time.h>

static const char *TAG = "schedule";

#define SCHEDULE_REFRESH_RETRY_DELAY_MS 5000

typedef struct {
    uint8_t slot_id;
    uint8_t hour;
    uint8_t minute;
    int day;
    bool triggered;
} TriggerRecord;

static void reset_trigger_records(TriggerRecord *records, size_t count) {
    memset(records, 0, count * sizeof(TriggerRecord));
}

static bool was_triggered_today(const TriggerRecord *records, size_t count, const SlotEntry *slot, int day) {
    for (size_t i = 0; i < count; i++) {
        if (records[i].triggered &&
            records[i].day == day &&
            records[i].slot_id == slot->slot_id &&
            records[i].hour == slot->hour &&
            records[i].minute == slot->minute) {
            return true;
        }
    }

    return false;
}

static void mark_triggered_today(TriggerRecord *records, size_t count, const SlotEntry *slot, int day) {
    for (size_t i = 0; i < count; i++) {
        if (!records[i].triggered) {
            records[i].slot_id = slot->slot_id;
            records[i].hour = slot->hour;
            records[i].minute = slot->minute;
            records[i].day = day;
            records[i].triggered = true;
            return;
        }
    }

    ESP_LOGW(TAG, "No room to record triggered slot: slot=%u time=%02u:%02u",
             (unsigned int)slot->slot_id,
             (unsigned int)slot->hour,
             (unsigned int)slot->minute);
}

static bool tick_has_reached(TickType_t now, TickType_t target) {
    const TickType_t half_range = (TickType_t)1 << ((sizeof(TickType_t) * 8) - 1);
    return (TickType_t)(now - target) < half_range;
}

static void consume_schedule_refresh_request(void) {
    static bool retry_pending = false;
    static uint32_t retry_request_count = 0;
    static TickType_t next_retry_tick = 0;

    TickType_t now_tick = xTaskGetTickCount();
    bool refresh_pending = false;
    uint32_t request_count = 0;

    if (retry_pending) {
        if (!tick_has_reached(now_tick, next_retry_tick)) {
            return;
        }
        refresh_pending = true;
        request_count = retry_request_count;
    } else {
        esp_err_t err = schedule_store_consume_refresh_request(&refresh_pending, &request_count);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to consume schedule refresh request: %s", esp_err_to_name(err));
            return;
        }
    }

    if (!refresh_pending) {
        return;
    }

    SlotEntry fetched_slots[SCHEDULE_SLOT_COUNT] = { 0 };
    size_t fetched_count = 0;

    // Fetch and apply the latest server schedule outside the MQTT event callback.
    esp_err_t err = schedule_fetcher_fetch(fetched_slots, SCHEDULE_SLOT_COUNT, &fetched_count);
    if (err == ESP_OK) {
        err = schedule_store_apply(fetched_slots, fetched_count);
    }

    if (err != ESP_OK) {
        retry_pending = true;
        retry_request_count = request_count;
        next_retry_tick = xTaskGetTickCount() + pdMS_TO_TICKS(SCHEDULE_REFRESH_RETRY_DELAY_MS);
        ESP_LOGW(TAG,
                 "Schedule refresh failed; keeping previous snapshot: request_count=%u err=%s",
                 (unsigned int)request_count,
                 esp_err_to_name(err));
        return;
    }

    retry_pending = false;
    ESP_LOGI(TAG,
             "Schedule refresh applied: request_count=%u count=%u",
             (unsigned int)request_count,
             (unsigned int)fetched_count);
}

void schedule_task(void *arg) {
    ESP_LOGI(TAG, "Schedule task started");

    // Wait for RTC synchronization before starting the schedule task
    while (!rtc_driver_is_synced()) {
        ESP_LOGI(TAG, "Waiting for RTC synchronization...");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    ESP_LOGI(TAG, "RTC synchronized, starting schedule monitoring");

    TriggerRecord trigger_records[SCHEDULE_SLOT_COUNT] = { 0 };
    uint32_t last_version = UINT32_MAX;
    int last_day = -1; // To track day changes and reset triggered flags

    // Main loop to check and trigger scheduled slots
    while (1) {
        consume_schedule_refresh_request();

        time_t now;
        // Get current time from RTC
        esp_err_t err = rtc_driver_get_time(&now);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to get RTC time: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        struct tm now_tm;
        localtime_r(&now, &now_tm);

        // Load the latest schedule snapshot
        ScheduleSnapshot snapshot;
        err = schedule_store_get_snapshot(&snapshot);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to load schedule snapshot: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // Check if the schedule version has changed, indicating an update to the schedule
        if (snapshot.version != last_version) {
            last_version = snapshot.version;
            ESP_LOGI(TAG, "Loaded schedule snapshot: count=%u version=%u",
                     (unsigned int)snapshot.count,
                     (unsigned int)snapshot.version);
        }

        if (now_tm.tm_mday != last_day) {
            ESP_LOGI(TAG, "Day changed, resetting triggered flags");

            reset_trigger_records(trigger_records, SCHEDULE_SLOT_COUNT);
            last_day = now_tm.tm_mday;
        }

        // Iterate through the schedule slots and trigger any that match the current time
        for (size_t i = 0; i < snapshot.count; i++) {
            const SlotEntry *slot = &snapshot.slots[i];
            uint8_t slot_id = slot->slot_id;

            if (slot_id == 0 || slot_id > SCHEDULE_SLOT_COUNT) {
                ESP_LOGW(TAG, "Skipping invalid schedule slot id: %u", (unsigned int)slot_id);
                continue;
            }

            if (was_triggered_today(trigger_records, SCHEDULE_SLOT_COUNT, slot, now_tm.tm_mday)) {
                continue; // Skip already triggered slots
            }

            // Check if the current time matches the scheduled time for this slot
            if (now_tm.tm_hour == slot->hour && now_tm.tm_min == slot->minute) {
                ESP_LOGI(TAG, "Slot matched! slot = %d, time = %02d:%02d", slot_id, slot->hour, slot->minute);
                
                // Enqueue dispense event for the matched slot
                err = dispense_enqueue(slot_id);
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Dispense event enqueued for slot %d", slot_id);
                    mark_triggered_today(trigger_records, SCHEDULE_SLOT_COUNT, slot, now_tm.tm_mday);
                } else {
                    ESP_LOGE(TAG, "Failed to enqueue dispense event for slot %d: %s", slot_id, esp_err_to_name(err));
                }
            }
        }    
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    } 
}
