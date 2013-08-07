list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_default_display_buffer_compositor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_stream.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_swapper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_swapper_double.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_swapper_triple.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_swapper_spin_triple.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_temporary_buffers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_swapper_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_rendering_operator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_multi_threaded_compositor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_switching_bundle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_rw_lock.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gl_renderer.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
