add_library(
  mirwayland SHARED

  wayland_wrapper.cpp                       wayland_wrapper.h
  xdg-shell-unstable-v6_wrapper.cpp         xdg-shell-unstable-v6_wrapper.h
  xdg-shell_wrapper.cpp                     xdg-shell_wrapper.h
  wlr-layer-shell-unstable-v1_wrapper.cpp   wlr-layer-shell-unstable-v1_wrapper.h
)

target_link_libraries(mirwayland LINK_PUBLIC
  mircore
  mircommon
  ${WAYLAND_SERVER_LDFLAGS} ${WAYLAND_SERVER_LIBRARIES}
)

target_include_directories(mirwayland
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# TODO ABI, publishing, debian/*.symbols