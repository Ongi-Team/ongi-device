#ifndef SCHEDULE_FETCHER_H
#define SCHEDULE_FETCHER_H

#include <stddef.h>

#include "esp_err.h"
#include "schedule.h"

esp_err_t schedule_fetcher_fetch(SlotEntry *slots, size_t max_count, size_t *out_count);

#endif // SCHEDULE_FETCHER_H
