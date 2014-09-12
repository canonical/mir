string (REPLACE " -Wl,--no-undefined" " " CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})

include_directories(
    ${DRM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

add_library(mirclientplatformmesaobjects OBJECT
  client_platform_factory.cpp
  client_platform.cpp
  client_buffer_factory.cpp
  client_buffer.cpp
  mesa_native_display_container.cpp
  native_surface.cpp
)

add_library(
  mirclientplatformmesa SHARED

  $<TARGET_OBJECTS:mirclientplatformmesaobjects>
  $<TARGET_OBJECTS:mirclient_platformimpl>
)

set_target_properties(
  mirclientplatformmesa PROPERTIES
  OUTPUT_NAME ${MIR_CLIENT_DRIVER}
  LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH}/mesa
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
)

target_link_libraries(
  mirclientplatformmesa

  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
)

install(TARGETS mirclientplatformmesa LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mir/${CLIENT_DRIVER}/mesa)

if (MIR_TEST_PLATFORM STREQUAL "mesa")
  add_custom_command(TARGET mirclientplatformmesa
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove ${MIR_CLIENT_DRIVER_BINARY}
    COMMAND ${CMAKE_COMMAND} -E create_symlink mesa/$<TARGET_FILE_NAME:mirclientplatformmesa> ${MIR_CLIENT_DRIVER_BINARY}
    WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH}
  )

  install(CODE
    "execute_process(
       COMMAND ln -sf mir/${CLIENT_DRIVER}/mesa/${MIR_CLIENT_DRIVER_BINARY}
       WORKING_DIRECTORY \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
     )"
  )
endif()

