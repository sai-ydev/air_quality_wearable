idf_component_register(
    SRCS
        "src/zmod4xxx.c"
        "src/zmod4510_config.c"
        "src/zmod4xxx_hal.c"
        "src/hs4xxx.c"
        "src/hs3xxx.c"
        "src/hsxxxx.c"
        "src/hal.c"
        "src/hal_esp32.c"
    INCLUDE_DIRS
        "include"
        "../../main"
    REQUIRES
        driver
)

# Link the precompiled algorithm libraries
target_link_libraries(${COMPONENT_LIB} INTERFACE 
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/lib_no2_o3.a"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/lib_zmod4xxx_cleaning.a"
)