idf_component_register(SRCS "air_quality.c"
                       "bme690/bme69x.c"
                       "bme690/i2c_interface.c"
                       "bme690/bsec_integration.c"
                       "bme690/bsec_selectivity.c"
                        INCLUDE_DIRS "." "bme690")   

# Link the prebuilt archive to the component
target_link_libraries(${COMPONENT_LIB} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/bme690/libalgobsec.a")

