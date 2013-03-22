list(APPEND UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/egl_mock_test.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)

set(MIRTEST_EGLMOCK_SOURCES 
    gl_mock.cpp
    egl_mock.cpp )

if (MIR_PLATFORM STREQUAL "android")
  list( APPEND MIRTEST_EGLMOCK_SOURCES hw_mock.cpp )
endif()

add_library(
  mirtest_eglmock STATIC

  ${MIRTEST_EGLMOCK_SOURCES} 
)
