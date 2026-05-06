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
if(CI_BUILD)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CI_BUILD)
    target_compile_options(${COMPONENT_LIB} PRIVATE 
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-parameter
    )
endif()