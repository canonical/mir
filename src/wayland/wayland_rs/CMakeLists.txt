add_rust_cxx_library(mirwayland_rs
  CRATE wayland_rs
  CXX_BRIDGE_SOURCE_FILE src/ffi.rs
  DEPENDS Cargo.toml build_script/main.rs build_script/protocol_parser.rs src/lib.rs src/wayland_server.rs
  INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
)

file(GLOB WAYLAND_RS_INCLUDE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp")

add_executable(
  mirwayland_rs-demo
  ${CMAKE_CURRENT_SOURCE_DIR}/example/main.cpp
  ${WAYLAND_RS_INCLUDE_SOURCES}
)

target_include_directories(mirwayland_rs-demo
  PRIVATE
  "$<TARGET_PROPERTY:mirwayland_rs,INTERFACE_INCLUDE_DIRECTORIES>")

target_link_libraries(mirwayland_rs-demo
  PRIVATE
  mirwayland_rs
  PkgConfig::WAYLAND_CLIENT)
