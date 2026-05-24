#include "rtc_internal.h"

#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"

static const char *TAG = "rtc_internal";
static volatile bool s_synced = false;
static bool s_sntp_initialized = false;

// Callback function called when time is synchronized
static void time_sync_callback(struct timeval *tv) {
    s_synced = true;
    ESP_LOGI(TAG, "NTP time synchronized");
}

// Initialize the internal RTC
esp_err_t internal_rtc_init(void) {
    s_synced = false;

    // Set timezone to KST (UTC+9) and apply it
    setenv("TZ", "KST-9", 1); 
    tzset();

    return ESP_OK;
}

// Synchronize RTC with NTP server
esp_err_t internal_rtc_sync(void) {
    // Initialize SNTP if not already initialized
    if (!s_sntp_initialized) {
        // Configure SNTP with the default settings and the specified NTP server
        esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
        sntp_config.sync_cb = time_sync_callback;

        // Initialize SNTP and check for errors
        esp_err_t ret = esp_netif_sntp_init(&sntp_config);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize SNTP: %s", esp_err_to_name(ret));
            return ret;
        }
        s_sntp_initialized = true;
    }
    
    esp_err_t ret = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(30000)); // Wait for synchronization to complete with a timeout
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "RTC synchronization failed: %s", esp_err_to_name(ret));
        return ret;
    }

    s_synced = true;
    return ESP_OK;
}

esp_err_t internal_rtc_get_time(time_t *now) {
    if (now == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    time(now);
    if (*now < 1577836800) { // Check if time is before Jan 1, 2020
        return ESP_ERR_INVALID_STATE; // Time not synchronized
    }

    return ESP_OK;
}

bool internal_rtc_is_synced(void) {
    return s_synced;
}