include_directories(
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/include/renderers/sw
  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/client

  ${CMAKE_SOURCE_DIR}
)

add_library(mir-protected-test-doubles OBJECT
  mock_input_device.cpp
  triggered_main_loop.cpp
  fake_alarm_factory.cpp
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/null_message_sender.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/mock_console_services.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/null_device_observer.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/simple_device_observer.h
  simple_device_observer.cpp
  stub_console_services.cpp
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/stub_console_services.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/null_gl_config.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/null_display_configuration_policy.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/stub_buffer_allocator.h
  stub_buffer_allocator.cpp
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/stub_gl_rendering_provider.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/mock_gl_rendering_provider.h
  ${PROJECT_SOURCE_DIR}/tests/include/mir/test/doubles/mock_output_surface.h
)

target_link_libraries(mir-protected-test-doubles
  PUBLIC
    mirplatform
    mircommon
    mircore
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  check_cxx_compiler_flag("-Wno-attributes=clang::" SUPPORTS_SCOPED_W_NO_ATTRIBUTES)
  if (SUPPORTS_SCOPED_W_NO_ATTRIBUTE)
    set_source_files_properties(
      stub_buffer_allocator.cpp
      PROPERTIES
        COMPILE_OPTIONS "-Wno-attributes=clang::"
    )
  else()
    set_source_files_properties(
      stub_buffer_allocator.cpp
      PROPERTIES
        COMPILE_OPTIONS "-Wno-attributes"
    )
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set_source_files_properties(
    stub_buffer_allocator.cpp
    PROPERTIES
      COMPILE_OPTIONS "-Wno-unknown-attributes"
  )
endif()

set(MIR_TEST_DOUBLES_UDEV_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/mock_udev.cpp)

set(
  MIR_TEST_DOUBLES_PLATFORM_SRCS

  ${CMAKE_CURRENT_SOURCE_DIR}/mock_libinput.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_egl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_gl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mock_drm.cpp
)

if (MIR_BUILD_PLATFORM_X11)
  include_directories(
          ${PROJECT_SOURCE_DIR}/src/platforms/gbm-kms/server
  )
  list(APPEND MIR_TEST_DOUBLES_PLATFORM_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_x11.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_xkb.cpp
  )
endif()

if (MIR_BUILD_PLATFORM_GBM_KMS OR MIR_BUILD_PLATFORM_ATOMIC_KMS)
  include_directories(
          ${PROJECT_SOURCE_DIR}/src/platforms/gbm-kms/server
    ${CMAKE_SOURCE_DIR}
  )
  list(APPEND MIR_TEST_DOUBLES_PLATFORM_SRCS
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

target_link_libraries(mir-public-test-doubles
  PUBLIC
    mirplatform
    mircommon
    mircore
)

add_library(mir-test-doubles-static STATIC
  $<TARGET_OBJECTS:mir-public-test-doubles>
  $<TARGET_OBJECTS:mir-protected-test-doubles>
)

target_include_directories(
  mir-test-doubles-static
  PUBLIC
    ${PROJECT_SOURCE_DIR}/tests/include
)

add_dependencies(mir-test-doubles-static GMock)

target_link_libraries(mir-test-doubles-static

  mirserver

  mir-test-framework-static

  Boost::system
  ${GMOCK_LIBRARIES}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

add_library(mir-public-test-doubles-platform OBJECT
  ${MIR_TEST_DOUBLES_PLATFORM_SRCS}
)

target_link_libraries(mir-public-test-doubles-platform
  PUBLIC
    mircommon
    mircore
  PRIVATE
    PkgConfig::DRM
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
