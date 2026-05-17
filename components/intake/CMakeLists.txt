idf_component_register(
    SRCS "dummy_intake_detector.c" "intake_detector_factory.c"
    INCLUDE_DIRS "include"
    REQUIRES )
if(CI_BUILD)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CI_BUILD)
    target_compile_options(${COMPONENT_LIB} PRIVATE 
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-parameter
    )
endif()