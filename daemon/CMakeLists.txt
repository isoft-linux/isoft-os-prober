include_directories(
    ${GLIB2_INCLUDE_DIRS} 
    ${GIO2_INCLUDE_DIRS}
    ${GIOUNIX_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

execute_process(COMMAND ${GDBUS_CODEGEN_EXECUTABLE} --generate-c-code
    ${CMAKE_CURRENT_BINARY_DIR}/os-prober-generated --c-namespace OSProber --interface-prefix org.isoftlinux.
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/org.isoftlinux.OSProber.xml
                        RESULT_VARIABLE codegen_failed)
if(codegen_failed)
    message(FATAL_ERROR "Error in generating code for os-prober-generated using gdbus-codegen")
endif()

add_executable(isoft-os-prober-daemon 
    main.c
    daemon.c
    extensions.c
    ${CMAKE_CURRENT_BINARY_DIR}/os-prober-generated.c
)

target_link_libraries(isoft-os-prober-daemon
    ${GLIB2_LIBRARIES}
    ${GIO2_LIBRARIES}
)

install(TARGETS isoft-os-prober-daemon RUNTIME DESTINATION bin)
