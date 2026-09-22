add_compile_definitions(MIR_LOG_COMPONENT_FALLBACK="evdev-input")

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

if (LIBINPUT_VERSION VERSION_GREATER_EQUAL 1.32)
  include(CheckCXXSourceCompiles)
  set(_mir_saved_cmake_required_includes "${CMAKE_REQUIRED_INCLUDES}")
  set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${LIBINPUT_INCLUDE_DIRS})
  check_cxx_source_compiles(
    "#include <libinput.h>
     int main()
     {
         auto method = LIBINPUT_CONFIG_CIRCULAR_SCROLL;
         return method == LIBINPUT_CONFIG_CIRCULAR_SCROLL;
     }"
    LIBINPUT_HAS_CONFIG_CIRCULAR_SCROLL)
  check_cxx_source_compiles(
    "#include <libinput.h>
     int main()
     {
         auto method = LIBINPUT_CONFIG_SCROLL_CIRCULAR;
         return method == LIBINPUT_CONFIG_SCROLL_CIRCULAR;
     }"
    LIBINPUT_HAS_CONFIG_SCROLL_CIRCULAR)
  set(CMAKE_REQUIRED_INCLUDES "${_mir_saved_cmake_required_includes}")
  unset(_mir_saved_cmake_required_includes)

  set(MIR_EVDEV_LIBINPUT_CIRCULAR_SCROLL_DEFINITIONS)
  if (LIBINPUT_HAS_CONFIG_CIRCULAR_SCROLL)
    list(APPEND MIR_EVDEV_LIBINPUT_CIRCULAR_SCROLL_DEFINITIONS
      LIBINPUT_HAS_CIRCULAR_SCROLL
      MIR_LIBINPUT_CIRCULAR_SCROLL_METHOD=LIBINPUT_CONFIG_CIRCULAR_SCROLL)
  elseif (LIBINPUT_HAS_CONFIG_SCROLL_CIRCULAR)
    list(APPEND MIR_EVDEV_LIBINPUT_CIRCULAR_SCROLL_DEFINITIONS
      LIBINPUT_HAS_CIRCULAR_SCROLL
      MIR_LIBINPUT_CIRCULAR_SCROLL_METHOD=LIBINPUT_CONFIG_SCROLL_CIRCULAR)
  endif()

  if (MIR_EVDEV_LIBINPUT_CIRCULAR_SCROLL_DEFINITIONS)
    target_compile_definitions(mirplatforminputevdevobjects
      PUBLIC
        ${MIR_EVDEV_LIBINPUT_CIRCULAR_SCROLL_DEFINITIONS})
  endif()
endif()

add_library(mirplatforminputevdev MODULE
  platform_factory.cpp
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
  PUBLIC
    mirplatform # udev wrapper
    mirevdevutilsobjects
    Boost::program_options
    PkgConfig::LIBINPUT
  PRIVATE
    mirplatforminputevdevobjects
)

install(TARGETS mirplatforminputevdev LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
