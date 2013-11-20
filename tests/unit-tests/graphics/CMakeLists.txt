list(APPEND UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_graphics_platform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_display_configuration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_egl_extensions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_display.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_default_display_configuration_policy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_id.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_properties.cpp
)

add_subdirectory(nested/)
add_subdirectory(offscreen/)

add_subdirectory(egl_mock/)
if (MIR_PLATFORM STREQUAL "android")
add_subdirectory(android/)
endif()

if (MIR_PLATFORM STREQUAL "gbm")
add_subdirectory(gbm/)
endif()

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
