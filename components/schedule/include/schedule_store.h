#ifndef SCHEDULE_STORE_H
#define SCHEDULE_STORE_H

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "schedule.h"

typedef struct {
    SlotEntry slots[SCHEDULE_SLOT_COUNT];
    size_t count; // Number of valid slots in the snapshot
    uint32_t version;
} ScheduleSnapshot;

esp_err_t schedule_store_init(void);
esp_err_t schedule_store_apply(const SlotEntry *slots, size_t count);
esp_err_t schedule_store_apply_fixture(void);
esp_err_t schedule_store_get_snapshot(ScheduleSnapshot *snapshot);
esp_err_t schedule_store_request_refresh(void);
esp_err_t schedule_store_consume_refresh_request(bool *was_pending, uint32_t *request_count);
esp_err_t schedule_store_clear(void);

#endif // SCHEDULE_STORE_H
