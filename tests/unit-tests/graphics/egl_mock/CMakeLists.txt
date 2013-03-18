list(APPEND UNIT_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/egl_mock_test.cpp
)

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)

set(C_MOCKS_SOURCES 
    gl_mock.cpp
    egl_mock.cpp )

if (MIR_PLATFORM STREQUAL "android")
  list( APPEND C_MOCKS_SOURCES hw_mock.cpp )
endif()

add_library(
  mirtest_eglmock STATIC

  ${C_MOCKS_SOURCES}
)
