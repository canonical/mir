include_directories(
  ${PROJECT_SOURCE_DIR}/include/common
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/server
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/tests/include/

  ${PROJECT_SOURCE_DIR}/src/include/platform
  ${PROJECT_SOURCE_DIR}
  ${MIR_ANDROID_INCLUDE_DIRECTORIES}
  ${MIR_3RD_PARTY_INCLUDE_DIRECTORIES}
)

add_executable(frame_uniformity_test_client
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
  
  mir-test
  mir-test-framework
  mir-test-doubles
  mir-test-doubles-platform

  # Why does this have to come after mir-test-framework here but not in tests/acceptance-tests/
  # /CMakeLists.txt? ~racarr
  mirplatform

  3rd_party
  ${PROTOBUF_LIBRARIES}
  ${Boost_LIBRARIES}
  ${GTEST_BOTH_LIBRARIES}
  ${GMOCK_LIBRARY}
  ${GMOCK_MAIN_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT} # Link in pthread.
)

uses_android_input(frame_uniformity_test_client)
