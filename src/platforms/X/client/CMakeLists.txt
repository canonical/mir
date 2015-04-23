include_directories(${client_common_include_dirs})

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

add_library(mirclientplatformXobjects OBJECT
  client_platform_factory.cpp
  client_platform.cpp
  client_buffer_factory.cpp
  client_buffer.cpp
)

add_library(
  mirclientplatformX MODULE

  $<TARGET_OBJECTS:mirclientplatformXobjects>
  $<TARGET_OBJECTS:client_platform_common>
)

set_target_properties(
  mirclientplatformX PROPERTIES
  OUTPUT_NAME X
  LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH}/client-modules
  PREFIX ""
  SUFFIX ".so.${MIR_CLIENT_PLATFORM_ABI}"
  LINK_FLAGS "-Wl,--version-script,${symbol_map}"
)

target_link_libraries(
  mirclientplatformX

  mirclient
  EGL
  X11
)

install(TARGETS mirclientplatformX LIBRARY DESTINATION ${MIR_CLIENT_PLATFORM_PATH})
