include_directories(${PROJECT_SOURCE_DIR}/include/server)

add_subdirectory(compositor/)
add_subdirectory(graphics/)
add_subdirectory(input/)
add_subdirectory(logging/)
add_subdirectory(surfaces/)
add_subdirectory(options/)
add_subdirectory(frontend/)
add_subdirectory(shell/)
add_subdirectory(time/)

set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mirserver")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirserver.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirserver.pc
)

set(
  SRCS
  run_mir.cpp
  display_server.cpp
  default_server_configuration.cpp
  signal_dispatcher.cpp
)

set(MIRSERVER_LINKAGE SHARED)

add_library(mirserver ${MIRSERVER_LINKAGE}
  ${SRCS}
)

add_executable(
  mirds

  main.cpp
)

set_target_properties(mirds PROPERTIES OUTPUT_NAME mir)

list(APPEND MIRSERVER_ARCHIVES
  miroptions
  mircompositor
  mirfrontend
  mirgraphics
  mirinput
  mirinputandroidtransport
  mirsurfaces
  mirtime
)

list(APPEND MIRSERVER_LINKS
  mirplatformgraphics
  mirprotobuf
  3rd_party
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
    LIBRARY DESTINATION lib
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

set_target_properties(mirserver PROPERTIES
  VERSION ${MIR_VERSION_MAJOR}.${MIR_VERSION_MINOR}.${MIR_VERSION_PATCH}
  SOVERSION ${MIR_VERSION_MAJOR}
)

target_link_libraries(mirds
  mirserver
  ${Boost_LIBRARIES}
)

install(TARGETS mirds
  RUNTIME DESTINATION bin
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mirserver.pc
  DESTINATION lib/pkgconfig
)
