idf_component_register(
    SRCS "schedule.c" "schedule_store.c"
    INCLUDE_DIRS "include"
    REQUIRES rtc dispense)
if(CI_BUILD)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CI_BUILD)
    target_compile_options(${COMPONENT_LIB} PRIVATE 
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-parameter
    )
endif()
