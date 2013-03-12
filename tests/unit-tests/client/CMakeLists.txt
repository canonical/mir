list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_client.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_client_platform.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_client_mir_surface.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_mir_connection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_wait_handle.cpp
)

if(MIR_PLATFORM STREQUAL "android")
list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_client_android_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_client_android_registrar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_native_window.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_client_depository.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_android_client_platform.cpp
)
endif()

if(MIR_PLATFORM STREQUAL "gbm")
list(APPEND UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_client_depository.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_client_buffer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test_gbm_client_platform.cpp
)
endif()

set(UNIT_TEST_SOURCES ${UNIT_TEST_SOURCES} PARENT_SCOPE)
