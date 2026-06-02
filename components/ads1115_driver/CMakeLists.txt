idf_component_register(
    SRCS "ads1115.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_driver_i2c freertos)
if(CI_BUILD)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CI_BUILD)
    target_compile_options(${COMPONENT_LIB} PRIVATE
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-parameter
    )
endif()
