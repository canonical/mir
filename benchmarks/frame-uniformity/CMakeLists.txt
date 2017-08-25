include_directories(
  ${PROJECT_SOURCE_DIR}/include/common
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/include/test
  ${PROJECT_SOURCE_DIR}/include/renderers/gl
  ${PROJECT_SOURCE_DIR}/include/renderers/sw

  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/src/include/common
  ${PROJECT_SOURCE_DIR}

  # needed for fake_event_hub_server_configuration.h (which relies on private APIs)
  ${PROJECT_SOURCE_DIR}/tests/include/
)

mir_add_wrapped_executable(frame_uniformity_test_client NOINSTALL
  touch_measuring_client.cpp
  touch_producing_server.cpp
  frame_uniformity_test.cpp
  vsync_simulating_graphics_platform.cpp
  touch_samples.cpp
  main.cpp
)

target_link_libraries(frame_uniformity_test_client
  mirserver
  mirclient
  mirplatform
  
  mir-test-assist

  # needed for fake_event_hub_server_configuration.h (which relies on private APIs)
  mir-test-framework-static

  # needed for vsync_simulating_graphics_platform.cpp
  mir-test-doubles-static

  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)
