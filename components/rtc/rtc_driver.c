#include "rtc_driver.h"
#include "rtc_internal.h"

static const char *TAG = "rtc_driver";

static const RtcDriver INTERNAL_RTC_DRIVER = {
    .init = internal_rtc_init,
    .sync = internal_rtc_sync,
    .get_time = internal_rtc_get_time,
    .is_synced = internal_rtc_is_synced
};

const RtcDriver *rtc_get_driver(void) {
    return &INTERNAL_RTC_DRIVER;
}

esp_err_t rtc_log_current_time(void) {
    time_t now;

    esp_err_t ret = rtc_get_driver()->get_time(&now);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get RTC time: %s", esp_err_to_name(ret));
        return ret;
    }

    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    ESP_LOGI(TAG, "Current RTC time: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    return ESP_OK;
}