list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_simple_dispatch_thread.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_multiplexing_dispatchable.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
