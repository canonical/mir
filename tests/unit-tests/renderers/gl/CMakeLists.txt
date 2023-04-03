list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gl_renderer.cpp
#  ${CMAKE_CURRENT_SOURCE_DIR}/test_basic_buffer_render_target.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
