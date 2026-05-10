idf_component_register(
    SRCS "rtc_driver.c" "rtc_internal.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_netif)
if(CI_BUILD)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CI_BUILD)
    target_compile_options(${COMPONENT_LIB} PRIVATE 
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-parameter
    )
endif()