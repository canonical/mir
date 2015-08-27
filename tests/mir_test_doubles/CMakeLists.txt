include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/client

    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

set(
  TEST_UTILS_SRCS

  mock_frame_dropping_policy_factory.cpp
  mock_timer.cpp
  platform_factory.cpp
  stub_buffer.cpp
  test_protobuf_client.cpp
  test_protobuf_socket_server.cpp
  triggered_main_loop.cpp
  fake_alarm_factory.cpp
)

set(
  MIR_TEST_DOUBLES_PLATFORM_SRCS

  ${CMAKE_CURRENT_SOURCE_DIR}/mock_egl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_gl.cpp
)

if (MIR_BUILD_PLATFORM_MESA_X11)
  include_directories(
    ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server/common
    ${DRM_INCLUDE_DIRS}
  )
  list(APPEND MIR_TEST_DOUBLES_PLATFORM_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_drm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_gbm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_x11.cpp
  )
endif()

if (MIR_BUILD_PLATFORM_MESA_KMS)
  include_directories(
    ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server/common
    ${DRM_INCLUDE_DIRS}
    ${GBM_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
  )
  list(APPEND MIR_TEST_DOUBLES_PLATFORM_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_drm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_gbm.cpp
  )
endif()

if (MIR_BUILD_PLATFORM_ANDROID)
  include_directories(SYSTEM ${ANDROID_HEADERS_INCLUDE_DIRS})
  list(APPEND MIR_TEST_DOUBLES_PLATFORM_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_android_hw.cpp
  )
endif()

add_library(mir-public-test-doubles OBJECT
  fake_event_hub.cpp
  nested_mock_egl.cpp
  null_logger.cpp
)

add_library(mir-test-doubles-static STATIC
  $<TARGET_OBJECTS:mir-public-test-doubles>
  ${TEST_UTILS_SRCS}
)

add_dependencies(mir-test-doubles-static GMock)

uses_android_input(mir-public-test-doubles)

target_link_libraries(mir-test-doubles-static

  mirserver

  ${Boost_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

add_library(mir-public-test-doubles-platform OBJECT
  ${MIR_TEST_DOUBLES_PLATFORM_SRCS}
)

add_library(
  mir-test-doubles-platform-static STATIC
  $<TARGET_OBJECTS:mir-public-test-doubles-platform>
)

target_link_libraries(
  mir-test-doubles-platform-static

  -ldl
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)
