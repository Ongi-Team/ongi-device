#include "schedule.h"
#include "rtc_driver.h"
#include "esp_log.h"
#include "esp_err.h"
#include <time.h>

static const char *TAG = "schedule";

static void init_test_slots(SlotEntry *slots, size_t count) {
    time_t now_sec;
    struct tm now;

    time(&now_sec);
    localtime_r(&now_sec, &now);
    
    ESP_LOGI(TAG, "init_test_slots called");

    for (int i=0; i<count; i++) {
        struct tm slot_time = now;
        slot_time.tm_min += i+1; // 1-minute intervals from current time

        mktime(&slot_time); // Normalize time structure

        slots[i].slot_id = i+1;
        slots[i].hour = slot_time.tm_hour;
        slots[i].minute = slot_time.tm_min;
        slots[i].triggered = false;

        ESP_LOGI(TAG, "test slot %d => %02d:%02d", slots[i].slot_id, slots[i].hour, slots[i].minute);
    }
}

esp_err_t schedule_init(SlotEntry *slots, size_t count) {
    if (slots == NULL || count == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    init_test_slots(slots, count);
    return ESP_OK;
}

void schedule_task(void *arg) {
    ESP_LOGI(TAG, "Schedule task started");

    // Wait for RTC synchronization before starting the schedule task
    while (!rtc_driver_is_synced()) {
        ESP_LOGI(TAG, "Waiting for RTC synchronization...");
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    ESP_LOGI(TAG, "RTC synchronized, starting schedule monitoring");

    // Initialize schedule slots (for testing, we create slots at 1-minute intervals from current time)
    SlotEntry slots[SCHEDULE_SLOT_COUNT];
    schedule_init(slots, SCHEDULE_SLOT_COUNT);

    // Main loop to check and trigger scheduled slots
    while (1) {
        time_t now;
        rtc_driver_get_time(&now);

        struct tm now_tm;
        localtime_r(&now, &now_tm);

        for (int i=0; i<SCHEDULE_SLOT_COUNT; i++) {
            SlotEntry *slot = &slots[i];

            if (slot->triggered) {
                continue; // Skip already triggered slots
            }

            if (now_tm.tm_hour == slot->hour && now_tm.tm_min == slot->minute) {
                ESP_LOGI(TAG, "Slot matched! slot = %d, time = %02d:%02d", slot->slot_id, slot->hour, slot->minute);
                slot->triggered = true;
            }
        }    
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    } 
}