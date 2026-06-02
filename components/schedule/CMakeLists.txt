idf_component_register(
    SRCS "schedule.c" "schedule_store.c" "schedule_fetcher.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "../wifi"
    REQUIRES rtc dispense json esp_http_client esp-tls wifi)
if(CI_BUILD)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CI_BUILD)
    target_compile_options(${COMPONENT_LIB} PRIVATE 
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-parameter
    )
endif()
if(TEST_BUILD)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE TEST_BUILD)
endif()
