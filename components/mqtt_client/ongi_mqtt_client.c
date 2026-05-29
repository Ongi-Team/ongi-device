#include "ongi_mqtt_client.h"

#include "esp_err.h"

#ifdef CI_BUILD
    #include "mqtt_config_example.h"
#else
    #include "mqtt_config.h"
#endif

esp_err_t ongi_mqtt_client_init(void) {
    return ESP_OK;
}

esp_err_t ongi_mqtt_client_start(void) {
    return ESP_ERR_NOT_SUPPORTED;
}
