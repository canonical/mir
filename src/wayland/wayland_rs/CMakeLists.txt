add_rust_cxx_library(mirwayland_rs
  CRATE wayland_rs
  DEPENDS Cargo.toml build.rs src/lib.rs src/wayland_server.rs
)

add_executable(
  mirwayland_rs-demo
  ${CMAKE_CURRENT_SOURCE_DIR}/example/main.cpp
)

target_link_libraries(mirwayland_rs-demo
  PRIVATE
  mirwayland_rs
  PkgConfig::WAYLAND_CLIENT)
