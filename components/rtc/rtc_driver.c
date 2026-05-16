#include "rtc_driver.h"
#include "rtc_internal.h"

static const char *TAG = "rtc_driver";

static const RtcDriver INTERNAL_RTC_DRIVER = {
    .init = internal_rtc_init,
    .sync = internal_rtc_sync,
    .get_time = internal_rtc_get_time,
    .is_synced = internal_rtc_is_synced
};

static TaskHandle_t s_rtc_sync_task_handle = NULL;
static esp_err_t rtc_log_current_time(void);

// Abstracted RTC driver interface implementation 
esp_err_t rtc_driver_init(void) {
    return INTERNAL_RTC_DRIVER.init();
}

esp_err_t rtc_driver_sync(void) {
    return INTERNAL_RTC_DRIVER.sync();
}

esp_err_t rtc_driver_get_time(time_t *now) {
    return INTERNAL_RTC_DRIVER.get_time(now);
}

bool rtc_driver_is_synced(void) {
    return INTERNAL_RTC_DRIVER.is_synced();
}

static void rtc_sync_task(void *arg) {
    esp_err_t ret = rtc_driver_sync();

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "RTC synchronized successfully");
        rtc_log_current_time();
    } else {
        ESP_LOGW(TAG, "RTC synchronization failed: %s", esp_err_to_name(ret));
    }

    s_rtc_sync_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t rtc_start_sync_task(void) {
    if (s_rtc_sync_task_handle != NULL) {
        return ESP_OK; // Task already running
    }

    BaseType_t ret = xTaskCreate(
        rtc_sync_task,
        "rtc_sync_task",
        4096,
        NULL,
        5,
        &s_rtc_sync_task_handle
    );

    if (ret != pdPASS) {
        s_rtc_sync_task_handle = NULL;
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t rtc_log_current_time(void) {
    time_t now;

    esp_err_t ret = rtc_driver_get_time(&now);
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