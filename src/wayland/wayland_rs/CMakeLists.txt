add_rust_cxx_library(mirwayland_rs
  CRATE wayland_rs
  DEPENDS Cargo.toml build.rs src/lib.rs src/wayland_server.rs
)
