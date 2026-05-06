idf_component_register(
    SRCS "storage_nvs.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash)
if(CI_BUILD)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CI_BUILD)
    target_compile_options(${COMPONENT_LIB} PRIVATE 
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-parameter
    )
endif()