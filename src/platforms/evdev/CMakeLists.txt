pkg_check_modules(LIBINPUT REQUIRED libinput)

include_directories(
  ${LIBINPUT_CFLAGS} 
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/platform # udev Context
  ${PROJECT_SOURCE_DIR}/src/include/common # raii.h
  ${PROJECT_SOURCE_DIR}/include/common
  ${PROJECT_SOURCE_DIR}/include/client
  )

add_library(mirevdevutilsobjects OBJECT
    evdev_device_detection.cpp
    button_utils.cpp
    )

add_library(mirplatforminputevdevobjects OBJECT
    libinput_device.cpp
    libinput_device_ptr.cpp
    libinput_ptr.cpp
    platform.cpp
    )

mir_add_library_with_symbols(mirplatforminputevdev MODULE
  ${MIR_INPUT_PLATFORM_VERSION_SCRIPT}

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
)

target_link_libraries(mirplatforminputevdev
  mirplatform # udev wrapper
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${LIBINPUT_LDFLAGS} ${LIBINPUT_LIBRARIES}
)

install(TARGETS mirplatforminputevdev LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
