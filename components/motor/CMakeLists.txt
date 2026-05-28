idf_component_register(
    SRCS "motor_task.c" "servo_driver_factory.c" "dummy_servo_driver.c"
    INCLUDE_DIRS "include"
    REQUIRES dispense intake event_queue esp_timer)
if(CI_BUILD)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CI_BUILD)
    target_compile_options(${COMPONENT_LIB} PRIVATE 
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-parameter
    )
endif()