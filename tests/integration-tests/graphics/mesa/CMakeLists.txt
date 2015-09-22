##############################################################################
# mir_integration_tests_mesa-kms
##############################################################################
mir_add_wrapped_executable(
  mir_integration_tests_mesa-kms
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_integration.cpp
  ${MIR_SERVER_OBJECTS}
  ${MIR_PLATFORM_OBJECTS}
  ${MIR_COMMON_OBJECTS}
  $<TARGET_OBJECTS:mirplatformgraphicsmesakmsobjects>
)

add_dependencies(mir_integration_tests_mesa-kms GMock)

target_link_libraries(
  mir_integration_tests_mesa-kms

  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  mirclient-static
  mirclientrpc-static
  mirclientlttng-static
  mirsharedmesaservercommon-static

  ${PROTOBUF_LITE_LIBRARIES}
  # Mesa platform dependencies
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
)

if (MIR_RUN_INTEGRATION_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_integration_tests_mesa-kms)
endif (MIR_RUN_INTEGRATION_TESTS)

##############################################################################
# mir_integration_tests_mesa-x11
##############################################################################
mir_add_wrapped_executable(
  mir_integration_tests_mesa-x11
  ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_integration.cpp
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/server/x11/X11_resources.cpp
  ${MIR_SERVER_OBJECTS}
  ${MIR_PLATFORM_OBJECTS}
  ${MIR_COMMON_OBJECTS}
  $<TARGET_OBJECTS:mirplatformgraphicsmesax11objects>
)

add_dependencies(mir_integration_tests_mesa-x11 GMock)

target_link_libraries(
  mir_integration_tests_mesa-x11

  mir-test-static
  mir-test-framework-static
  mir-test-doubles-static
  mirclient-static
  mirclientrpc-static
  mirclientlttng-static
  mirsharedmesaservercommon-static

  ${PROTOBUF_LITE_LIBRARIES}
  # Mesa platform dependencies
  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
  ${GBM_LDFLAGS} ${GBM_LIBRARIES}
  X11
)

if (MIR_RUN_INTEGRATION_TESTS)
  mir_discover_tests_with_fd_leak_detection(mir_integration_tests_mesa-x11)
endif (MIR_RUN_INTEGRATION_TESTS)
