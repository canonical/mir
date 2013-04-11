list(APPEND UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_graphics_platform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_gl_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_display_configuration.cpp
)

add_subdirectory(egl_mock/)
if (MIR_PLATFORM STREQUAL "android")
add_subdirectory(android/)
endif()

if (MIR_PLATFORM STREQUAL "gbm")
add_subdirectory(gbm/)
add_subdirectory(egl)
endif()

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
