include_directories(${client_common_include_dirs})
include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})
add_definitions(-DANDROID)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

add_library(mirclientplatformandroidobjects OBJECT
  buffer.cpp
  android_client_buffer_factory.cpp
  gralloc_registrar.cpp
  android_client_platform.cpp
  client_platform_factory.cpp
  egl_native_surface_interpreter.cpp
  android_native_display_container.cpp
)

add_library(
  mirclientplatformandroid MODULE

  $<TARGET_OBJECTS:mirclientplatformandroidobjects>
  $<TARGET_OBJECTS:client_platform_common>
)

set_target_properties(
  mirclientplatformandroid PROPERTIES
  OUTPUT_NAME android
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/client-modules
  PREFIX ""
  SUFFIX ".so.${MIR_CLIENT_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--version-script,${symbol_map}"
)

target_link_libraries(mirclientplatformandroid
  mirclient
  mirsharedandroid-static
  ${LIBHARDWARE_LIBRARIES}
)

install(TARGETS mirclientplatformandroid LIBRARY DESTINATION ${MIR_CLIENT_PLATFORM_PATH})
