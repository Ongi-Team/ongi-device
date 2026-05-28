idf_component_register(
    SRCS "ongi_mqtt_client.c"
    INCLUDE_DIRS "include"
)

if(CI_BUILD)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CI_BUILD)
    target_compile_options(${COMPONENT_LIB} PRIVATE
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-parameter
    )
endif()
