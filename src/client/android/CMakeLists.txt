string (REPLACE " -Wl,--no-undefined" " " CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS})
include_directories(SYSTEM ${LIBHARDWARE_INCLUDE_DIRS})
add_definitions(-DANDROID)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

add_library(mirclientplatformandroidobjects OBJECT
  buffer.cpp
  android_client_buffer_factory.cpp
  gralloc_registrar.cpp
  android_client_platform.cpp
  client_platform_factory.cpp
  client_surface_interpreter.cpp
  android_native_display_container.cpp
)

add_library(
  mirclientplatformandroid SHARED
  
  $<TARGET_OBJECTS:mirclientplatformandroidobjects>
  $<TARGET_OBJECTS:mirclient_platformimpl>
)

set_target_properties(
  mirclientplatformandroid PROPERTIES
  OUTPUT_NAME mirclientplatform
  LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH}/android
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
)

target_link_libraries(
  mirclientplatformandroid
  mirsharedandroid

  ${LIBHARDWARE_LIBRARIES}
)

install(TARGETS mirclientplatformandroid LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mir/clientplatform/android)

if (MIR_TEST_PLATFORM STREQUAL "android")
  add_custom_command(TARGET mirclientplatformandroid
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove libmirclientplatform.so
    COMMAND ${CMAKE_COMMAND} -E create_symlink android/$<TARGET_FILE_NAME:mirclientplatformandroid> libmirclientplatform.so
    WORKING_DIRECTORY ${LIBRARY_OUTPUT_PATH}
  )

  install(CODE
    "execute_process(
       COMMAND ln -sf mir/clientplatform/android/libmirclientplatform.so
       WORKING_DIRECTORY \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
     )"
  )
endif()
