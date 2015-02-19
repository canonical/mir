find_package(PkgConfig)
# Make FreeType optional? We have a stub renderer to fall back on...
pkg_search_module(FREETYPE REQUIRED freetype2)

include_directories(${FREETYPE_INCLUDE_DIRS})

add_library(gltext STATIC
  gltext.cpp
  gltext_stub_renderer.cpp
  gltext_freetype_renderer.cpp
)
target_link_libraries(gltext ${FREETYPE_LIBRARIES})

add_library(demo-shell STATIC
  demo_compositor.cpp
  demo_renderer.cpp
  window_manager.cpp
)
target_link_libraries(demo-shell gltext)

mir_add_wrapped_executable(mir_proving_server
  demo_shell.cpp
)

target_link_libraries(mir_proving_server
  demo-shell
  mirserver
  playgroundserverconfig
  exampleserverconfig
)
