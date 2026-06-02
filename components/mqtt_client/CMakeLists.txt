idf_component_register(
    SRCS "ongi_mqtt_client.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "../wifi"
    PRIV_REQUIRES mqtt wifi motor schedule
)

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
