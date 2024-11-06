include_directories(
  ${LIBINPUT_CFLAGS} 
  ${PROJECT_SOURCE_DIR}/src/include/platform # udev Context
  )

add_definitions(-DMIR_LOG_COMPONENT_FALLBACK="evdev-input")

add_library(mirevdevutilsobjects OBJECT
    button_utils.cpp
)

target_link_libraries(mirevdevutilsobjects
  PUBLIC
    mircommon
    mircore
    PkgConfig::LIBINPUT
)

add_library(mirplatforminputevdevobjects OBJECT
    libinput_device.cpp
    libinput_device_ptr.cpp
    libinput_ptr.cpp
    platform.cpp
        fd_store.cpp fd_store.h)

target_link_libraries(mirplatforminputevdevobjects
  PUBLIC
    mirplatform
    mircommon
    mircore
    PkgConfig::LIBINPUT
)

add_library(mirplatforminputevdev MODULE
  platform_factory.cpp
  $<TARGET_OBJECTS:mirplatforminputevdevobjects>
  $<TARGET_OBJECTS:mirevdevutilsobjects>
)

set_target_properties(
  mirplatforminputevdev PROPERTIES
  OUTPUT_NAME input-evdev
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  PREFIX ""
  SUFFIX ".so.${MIR_SERVER_INPUT_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${MIR_INPUT_PLATFORM_VERSION_SCRIPT}"
  LINK_DEPENDS ${MIR_INPUT_PLATFORM_VERSION_SCRIPT}
)

target_link_libraries(mirplatforminputevdev
  mirplatform # udev wrapper
  Boost::program_options
  PkgConfig::LIBINPUT
)

install(TARGETS mirplatforminputevdev LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
