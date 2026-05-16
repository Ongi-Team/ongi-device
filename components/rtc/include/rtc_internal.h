#ifndef RTC_INTERNAL_H
#define RTC_INTERNAL_H

#include <stdbool.h>
#include <time.h>
#include "esp_err.h"

esp_err_t internal_rtc_init(void);
esp_err_t internal_rtc_sync(void);
esp_err_t internal_rtc_get_time(time_t *now);
bool internal_rtc_is_synced(void);

#endif // RTC_INTERNAL_H