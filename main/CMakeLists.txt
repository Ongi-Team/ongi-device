set(MAIN_REQUIRES storage wifi rtc schedule dispense motor event_queue intake)

if(NOT CI_BUILD AND NOT TEST_BUILD)
    list(APPEND MAIN_REQUIRES mqtt_client)
endif()

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES ${MAIN_REQUIRES})

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
