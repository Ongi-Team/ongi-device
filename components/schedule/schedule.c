#include "schedule.h"
#include "esp_log.h"
#include <time.h>

static const char *TAG = "schedule";

void init_test_slots(SlotEntry *slots, size_t count) {
    time_t now_sec;
    struct tm now;

    time(&now_sec);
    localtime_r(&now_sec, &now);

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