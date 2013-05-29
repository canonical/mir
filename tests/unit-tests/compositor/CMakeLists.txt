list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_default_compositing_strategy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_manager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_bundle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_swapper.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_swapper_double.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_swapper_triple.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_swapper_spin_triple.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_temporary_buffers.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_id.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_properties.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_swapper_factory.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_rendering_operator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_multi_threaded_compositor.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
