include_directories(${PROJECT_SOURCE_DIR}/include/server)

add_subdirectory(compositor/)
add_subdirectory(graphics/)
add_subdirectory(input/)
add_subdirectory(logging/)
add_subdirectory(surfaces/)
add_subdirectory(options/)
add_subdirectory(frontend/)
add_subdirectory(shell/)

set(MIR_SERVER_INCLUDE_DIR_SUFFIX "mir/server/")

set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/${MIR_SERVER_INCLUDE_DIR_SUFFIX}")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirserver.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirserver.pc
)

set(
  SRCS
  display_server.cpp
  default_server_configuration.cpp
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
  mirsurfaces
)

list(APPEND MIRSERVER_LINKS
  mirplatformgraphics
  mirprotobuf
  3rd_party
  ${Boost_LIBRARIES}
)

set(
  SERVER_PUBLIC_HEADERS

  ${CMAKE_SOURCE_DIR}/include/server/mir/graphics/drm_authenticator.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/graphics/buffer_initializer.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/graphics/platform.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/graphics/renderable.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/graphics/display_configuration.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/graphics/null_display_report.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/graphics/display_report.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/graphics/renderer.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/graphics/platform_ipc_package.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/graphics/gl_renderer.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/graphics/viewable_area.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/graphics/display_buffer.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/graphics/framebuffer_backend.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/graphics/display.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/time_source.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/input/event_filter.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/input/cursor_listener.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/input/input_manager.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/surface_creation_parameters.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/consuming_placement_strategy.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/session.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/organising_surface_factory.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/single_visibility_focus_mechanism.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/surface.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/surface_factory.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/session_store.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/application_session.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/session_container.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/focus_sequence.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/surface_id.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/session_manager.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/placement_strategy.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/focus_setter.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/shell/registration_order_focus_sequence.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/surfaces/surface_stack.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/surfaces/buffer_bundle.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/surfaces/surface.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/surfaces/buffer_bundle_factory.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/surfaces/surface_stack_model.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/surfaces/surface_controller.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/surfaces/graphic_region.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/server_configuration.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/buffer_allocation_strategy.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/buffer_id.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/drawer.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/buffer_properties.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/buffer_basic.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/buffer_ipc_package.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/graphic_buffer_allocator.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/buffer.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/buffer_swapper.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/buffer_bundle_surfaces.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/render_view.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/swapper_factory.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/buffer_swapper_multi.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/rendering_operator.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/compositor.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/compositor/buffer_bundle_manager.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/options/program_option.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/options/option.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/int_wrapper.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/display_server.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/default_server_configuration.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/logging/logger.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/logging/dumb_console_logger.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/logging/application_mediator_report.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/logging/display_report.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/frontend/application_mediator.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/frontend/protobuf_ipc_factory.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/frontend/communicator.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/frontend/application_mediator_report.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/frontend/resource_cache.h
  ${CMAKE_SOURCE_DIR}/include/server/mir/cached_ptr.h
)

set_target_properties(mirserver PROPERTIES
  VERSION ${MIR_VERSION_MAJOR}.${MIR_VERSION_MINOR}.${MIR_VERSION_PATCH}
  SOVERSION ${MIR_VERSION_MAJOR}
  PUBLIC_HEADER "${SERVER_PUBLIC_HEADERS}"
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
    PUBLIC_HEADER DESTINATION "include/${MIR_SERVER_INCLUDE_DIR_SUFFIX}"
  )
else()
  target_link_libraries(mirserver
    LINK_PUBLIC
      ${MIRSERVER_ARCHIVES}
      ${MIRSERVER_LINKS}
  )
endif()

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
