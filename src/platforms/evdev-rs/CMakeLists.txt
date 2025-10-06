include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/core
  ${PROJECT_SOURCE_DIR}/src/include/platform
)

add_definitions(-DMIR_LOG_COMPONENT_FALLBACK="evdev-input-rs")

add_library(mirplatforminputevdevrsobjects OBJECT
  platform.cpp
  platform_bridge.cpp platform_bridge.h
)

target_link_libraries(mirplatforminputevdevrsobjects
  PUBLIC
  mirplatform
  mircommon
  mircore
)

add_library(mirplatforminputevdevrs MODULE
  platform_factory.cpp
)

set_target_properties(
  mirplatforminputevdevrs PROPERTIES
  OUTPUT_NAME input-evdev-rs
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  PREFIX ""
  SUFFIX ".so.${MIR_SERVER_INPUT_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${MIR_INPUT_PLATFORM_VERSION_SCRIPT}"
  LINK_DEPENDS ${MIR_INPUT_PLATFORM_VERSION_SCRIPT}
)

target_link_libraries(mirplatforminputevdevrs
  PUBLIC
  mirplatform # udev wrapper
  mirevdevutilsobjects
  Boost::program_options
  PkgConfig::LIBINPUT
  PRIVATE
  mirplatforminputevdevrsobjects
)

add_rust_cxx_library(mirplatforminputevdevrs_rust
  CRATE mir_platforms_evdev_rs
  DEPENDS Cargo.toml src/lib.rs
  LIBRARIES PkgConfig::UDEV PkgConfig::LIBINPUT)

target_link_libraries(mirplatforminputevdevrsobjects
  PRIVATE
  mirplatforminputevdevrs_rust
)

install(TARGETS mirplatforminputevdevrs LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
