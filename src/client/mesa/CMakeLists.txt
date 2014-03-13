include_directories(
    ${DRM_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
)

add_library(
  mirclientplatformmesa SHARED

  client_platform_factory.cpp
  client_platform.cpp
  client_buffer_factory.cpp
  client_buffer.cpp
  mesa_native_display_container.cpp
  native_surface.cpp
)

set_target_properties(
  mirclientplatformmesa PROPERTIES
  OUTPUT_NAME mirclientplatform
  LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH}/mesa
)

target_link_libraries(
  mirclientplatformmesa
  mirclient

  ${DRM_LDFLAGS} ${DRM_LIBRARIES}
)

install(TARGETS mirclientplatformmesa LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mir/clientplatform/mesa)

if (MIR_TEST_PLATFORM STREQUAL "mesa")
  add_custom_command(TARGET mirclientplatformmesa
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove libmirclientplatform.so
    COMMAND ${CMAKE_COMMAND} -E create_symlink mesa/$<TARGET_FILE_NAME:mirclientplatformmesa> libmirclientplatform.so
    WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH}
  )

  install(CODE
    "execute_process(
       COMMAND ln -sf mir/clientplatform/mesa/libmirclientplatform.so
       WORKING_DIRECTORY \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
     )"
  )
endif()

