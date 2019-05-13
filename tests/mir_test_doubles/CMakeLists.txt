include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include/renderers/sw
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/src/include/cookie

    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

set(
  TEST_UTILS_SRCS

  mock_input_device.cpp
  mock_timer.cpp
  test_protobuf_client.cpp
  test_protobuf_socket_server.cpp
  triggered_main_loop.cpp
  fake_alarm_factory.cpp
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/null_message_sender.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/mock_message_sender.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/null_event_sink_factory.h
  null_event_sink_factory.cpp
  mock_event_sink_factory.cpp
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/mock_event_sink_factory.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/mock_console_services.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/null_device_observer.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/simple_device_observer.h
  simple_device_observer.cpp
  stub_console_services.cpp
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/stub_console_services.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/null_gl_config.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/null_display_configuration_policy.h
)

set(MIR_TEST_DOUBLES_UDEV_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/mock_udev.cpp)
if(MIR_LIBDRM_HAS_IS_MASTER)
  set_source_files_properties(mock_drm.cpp PROPERTIES COMPILE_DEFINITIONS MIR_LIBDRM_HAS_IS_MASTER)
endif()


set(
  MIR_TEST_DOUBLES_PLATFORM_SRCS

  ${CMAKE_CURRENT_SOURCE_DIR}/mock_libinput.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_egl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_gl.cpp
)

if (MIR_BUILD_PLATFORM_MESA_X11)
  include_directories(
    ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server
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
    ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server
    ${DRM_INCLUDE_DIRS}
    ${GBM_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
  )
  list(APPEND MIR_TEST_DOUBLES_PLATFORM_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_drm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_gbm.cpp
  )
endif()

# We don't want LTO objects in a published archive (libmir-test-assist.a)
string (REPLACE " -flto " " " CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
string (REPLACE " -flto " " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

add_library(mir-public-test-doubles OBJECT
  nested_mock_egl.cpp
  null_logger.cpp
  stub_display_configuration.cpp ${CMAKE_SOURCE_DIR}/include/test/mir/test/doubles/stub_display_configuration.h
  fake_display.cpp ${CMAKE_SOURCE_DIR}/include/test/mir/test/doubles/fake_display.h
)

add_library(mir-test-doubles-static STATIC
  $<TARGET_OBJECTS:mir-public-test-doubles>
  ${TEST_UTILS_SRCS}
)

add_dependencies(mir-test-doubles-static GMock)

target_link_libraries(mir-test-doubles-static

  mirserver

  mir-test-framework-static

  ${Boost_LIBRARIES}
  ${GMOCK_LIBRARIES}
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
  ${GMOCK_LIBRARIES}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

add_library(mir-test-doubles-udev OBJECT
  ${MIR_TEST_DOUBLES_UDEV_SRCS}
)
