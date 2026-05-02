idf_component_register(SRCS "storage_nvs.c" "wifi_heartbeat.c" "main.c"
                    INCLUDE_DIRS ".")

if(CI_BUILD)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CI_BUILD)
    target_compile_options(${COMPONENT_LIB} PRIVATE 
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-parameter
    )
endif()
