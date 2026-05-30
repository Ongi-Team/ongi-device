#include "schedule.h"
#include "schedule_store.h"
#include "rtc_driver.h"
#include "dispense.h"
#include "esp_log.h"
#include "esp_err.h"
#include <string.h>
#include <time.h>

static const char *TAG = "schedule";

static void reset_trigger_flags(bool *triggered_slots, size_t count) {
    memset(triggered_slots, 0, count * sizeof(bool));
}

void schedule_task(void *arg) {
    ESP_LOGI(TAG, "Schedule task started");

    // Wait for RTC synchronization before starting the schedule task
    while (!rtc_driver_is_synced()) {
        ESP_LOGI(TAG, "Waiting for RTC synchronization...");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    ESP_LOGI(TAG, "RTC synchronized, starting schedule monitoring");

    bool triggered_slots[SCHEDULE_SLOT_COUNT + 1] = { false };
    uint32_t last_version = UINT32_MAX;
    int last_day = -1; // To track day changes and reset triggered flags

    // Main loop to check and trigger scheduled slots
    while (1) {
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
            reset_trigger_flags(triggered_slots, SCHEDULE_SLOT_COUNT + 1);
            last_version = snapshot.version;
            ESP_LOGI(TAG, "Loaded schedule snapshot: count=%u version=%u",
                     (unsigned int)snapshot.count,
                     (unsigned int)snapshot.version);
        }

        if (now_tm.tm_mday != last_day) {
            ESP_LOGI(TAG, "Day changed, resetting triggered flags");

            reset_trigger_flags(triggered_slots, SCHEDULE_SLOT_COUNT + 1);
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

            if (triggered_slots[slot_id]) {
                continue; // Skip already triggered slots
            }

            // Check if the current time matches the scheduled time for this slot
            if (now_tm.tm_hour == slot->hour && now_tm.tm_min == slot->minute) {
                ESP_LOGI(TAG, "Slot matched! slot = %d, time = %02d:%02d", slot_id, slot->hour, slot->minute);
                
                // Enqueue dispense event for the matched slot
                err = dispense_enqueue(slot_id);
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Dispense event enqueued for slot %d", slot_id);
                    triggered_slots[slot_id] = true; // Mark slot as triggered to prevent retriggering within the same day
                } else {
                    ESP_LOGE(TAG, "Failed to enqueue dispense event for slot %d: %s", slot_id, esp_err_to_name(err));
                }
            }
        }    
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    } 
}
