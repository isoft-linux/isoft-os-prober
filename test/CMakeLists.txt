include_directories(
    ${GLIB2_INCLUDE_DIRS} 
    ${GIO2_INCLUDE_DIRS}
    ${GIOUNIX_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(test-os-prober 
    test-os-prober.c
)

target_link_libraries(test-os-prober
    ${GLIB2_LIBRARIES}
    ${GIO2_LIBRARIES}
)
