idf_component_register(
    SRCS
        "src/bme69x.c"
        "src/bsec_iaq.c"
        "src/bsec_integration.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        driver
)

# Link the BSEC2 precompiled library
target_link_libraries(${COMPONENT_LIB} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib/libalgobsec.a")