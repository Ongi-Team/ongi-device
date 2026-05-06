idf_component_register(
    SRCS "wifi_heartbeat.c"
    INCLUDE_DIRS "include"
    REQUIRES storage
        esp_wifi
        esp_timer
        esp_http_client
        nvs_flash
        mbedtls
        freertos
)