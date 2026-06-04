idf_component_register(
    SRCS "event_sender.c"
    INCLUDE_DIRS "include"
    REQUIRES event_model
        esp_http_client
        mbedtls
        wifi)
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
