include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/server)

add_subdirectory(compositor/)
add_subdirectory(graphics/)
add_subdirectory(input/)
add_subdirectory(logging/)
add_subdirectory(lttng/)
add_subdirectory(surfaces/)
add_subdirectory(options/)
add_subdirectory(frontend/)
add_subdirectory(shell/)
add_subdirectory(time/)

set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mirserver")
set(PLATFORM_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mirplatform")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirserver.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirserver.pc
)

set(
  SRCS
  run_mir.cpp
  report_exception.cpp
  display_server.cpp
  default_server_configuration.cpp
  asio_main_loop.cpp
  shared_library.cpp
)

set(MIRSERVER_LINKAGE SHARED)

add_library(mirserver ${MIRSERVER_LINKAGE}
  ${SRCS}
)

list(APPEND MIRSERVER_ARCHIVES
  miroptions
  mircompositor
  mirfrontend
  mirplatform
  mirinput
  mirsharedinput
  mirsharedlogging
  mirsurfaces
  mirtime
  mirlttng
)

list(APPEND MIRSERVER_LINKS
  mirprotobuf
  mirnestedgraphics
  mirclient
  3rd_party
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
)

if(${MIRSERVER_LINKAGE} STREQUAL SHARED)
  target_link_libraries(mirserver
    LINK_PRIVATE
      -Wl,-whole-archive
      ${MIRSERVER_ARCHIVES}
      -Wl,-no-whole-archive
    LINK_PUBLIC
      ${MIRSERVER_LINKS}
  )
  install(TARGETS mirserver
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
  install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/include/platform/mir DESTINATION "include/mirplatform"
  )
  install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/include/server/mir DESTINATION "include/mirserver"
  )
else()
  target_link_libraries(mirserver
    LINK_PUBLIC
      ${MIRSERVER_ARCHIVES}
      ${MIRSERVER_LINKS}
  )
endif()

set(MIRSERVER_ABI 7)

set_target_properties(
  mirserver

  PROPERTIES
  SOVERSION ${MIRSERVER_ABI}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mirserver.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
