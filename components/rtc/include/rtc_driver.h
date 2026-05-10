#ifndef _RTC_DRIVER_H_
#define _RTC_DRIVER_H_

#include <stdbool.h>
#include <time.h>
#include "esp_err.h"
#include "esp_log.h"

typedef struct {
    esp_err_t (*init)(void);
    esp_err_t (*sync)(void);
    esp_err_t (*get_time)(time_t *time);
    bool (*is_synced)(void);
} RtcDriver;

// Abstract RTC driver interface
const RtcDriver *rtc_get_driver(void);

// Utility function to log the current RTC time in a human-readable format
esp_err_t rtc_log_current_time(void);

#endif // _RTC_DRIVER_H_