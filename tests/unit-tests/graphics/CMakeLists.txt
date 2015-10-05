list(APPEND UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_display_configuration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_egl_extensions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_egl_error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_default_display_configuration_policy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_id.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_properties.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_pixel_format_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_surfaceless_egl_context.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_overlapping_output_grouping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_platform_prober.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_software_cursor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_graphics_platform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_display.cpp
)

add_subdirectory(nested/)
add_subdirectory(offscreen/)

add_subdirectory(egl_mock/)
if (MIR_TEST_PLATFORM STREQUAL "android")
  add_subdirectory(android/)
endif()

if (MIR_TEST_PLATFORM STREQUAL "mesa-kms" OR MIR_TEST_PLATFORM STREQUAL "mesa-x11")
  add_subdirectory(mesa/)
endif()

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
