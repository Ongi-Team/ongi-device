#include "storage_nvs.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"

// nvs initialization
esp_err_t nvs_init() {
    // Initialize NVS flash storage
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Erase NVS flash and check for errors
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            ESP_LOGE("NVS", "Failed to erase NVS flash: %s", esp_err_to_name(ret));
            return ret;
        }
        // Retry NVS flash initialization and check for errors
        ret = nvs_flash_init();
    }

    return ret;
}