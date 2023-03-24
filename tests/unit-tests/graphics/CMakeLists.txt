list(APPEND UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_display_configuration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_egl_extensions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_egl_error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_default_display_configuration_policy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_gamma_curves.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_id.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_properties.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_pixel_format_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_overlapping_output_grouping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_software_cursor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_anonymous_shm_file.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_shm_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_multiplexing_display.cpp
)

list(APPEND UMOCK_UNIT_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test_platform_prober.cpp)

add_subdirectory(egl_mock/)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
set(UMOCK_UNIT_TEST_SOURCES ${UMOCK_UNIT_TEST_SOURCES} PARENT_SCOPE)
