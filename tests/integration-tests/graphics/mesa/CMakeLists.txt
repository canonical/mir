if (MIR_BUILD_PLATFORM_MESA_KMS)
    ##########################################################################
    # mir_integration_tests_mesa-kms
    ##########################################################################
    mir_add_wrapped_executable(
      mir_integration_tests_mesa-kms
      ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_integration.cpp
    )

    add_dependencies(mir_integration_tests_mesa-kms GMock)

    target_link_libraries(
      mir_integration_tests_mesa-kms
    
      mir-test-static
      mir-test-framework-static
      mir-test-doubles-static
      mirclient-static
      mirclientlttng-static
      mirsharedmesaservercommon-static
      mircommon

      ${PROTOBUF_LITE_LIBRARIES}
      # Mesa platform dependencies
      ${DRM_LDFLAGS} ${DRM_LIBRARIES}
      ${GBM_LDFLAGS} ${GBM_LIBRARIES}
    )

    if (MIR_RUN_INTEGRATION_TESTS)
      mir_discover_tests_with_fd_leak_detection(mir_integration_tests_mesa-kms)
    endif (MIR_RUN_INTEGRATION_TESTS)
endif()

if (MIR_BUILD_PLATFORM_MESA_X11)
    ##########################################################################
    # mir_integration_tests_mesa-x11
    ##########################################################################
    mir_add_wrapped_executable(
      mir_integration_tests_mesa-x11
      ${CMAKE_CURRENT_SOURCE_DIR}/test_buffer_integration.cpp
      $<TARGET_OBJECTS:mirplatformservermesax11sharedresources>
    )

    add_dependencies(mir_integration_tests_mesa-x11 GMock)

    target_link_libraries(
      mir_integration_tests_mesa-x11
    
      mir-test-static
      mir-test-framework-static
      mir-test-doubles-static
      mirclient-static
      mirclientlttng-static
      mirsharedmesaservercommon-static
      mircommon

      ${PROTOBUF_LITE_LIBRARIES}
      # Mesa platform dependencies
      ${DRM_LDFLAGS} ${DRM_LIBRARIES}
      ${GBM_LDFLAGS} ${GBM_LIBRARIES}
      X11
    )

    if (MIR_RUN_INTEGRATION_TESTS)
      mir_discover_tests_with_fd_leak_detection(mir_integration_tests_mesa-x11)
    endif (MIR_RUN_INTEGRATION_TESTS)
endif()
