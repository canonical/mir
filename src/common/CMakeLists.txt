add_compile_definitions(MIR_LOG_COMPONENT_FALLBACK="mircommon")

add_subdirectory(events)
add_subdirectory(input)
add_subdirectory(logging)
add_subdirectory(report/lttng)
add_subdirectory(sharedlibrary)
add_subdirectory(thread)
add_subdirectory(time)
add_subdirectory(dispatch)

# TODO we need a place to manage ABI and related versioning but use this as placeholder
set(MIRCOMMON_ABI 11)
set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

add_library(mircommon SHARED
  default_font.cpp
  event.cpp
  event_printer.cpp
  immediate_executor.cpp
  libname.cpp                       ${PROJECT_SOURCE_DIR}/include/common/mir/libname.h
  linearising_executor.cpp
  log.cpp
  mir_cursor_api.cpp
  output_type_names.cpp
  posix_rw_mutex.cpp                ${PROJECT_SOURCE_DIR}/include/common/mir/posix_rw_mutex.h
  signal.cpp                        ${PROJECT_SOURCE_DIR}/include/common/mir/signal.h
  errno_utils.cpp
  thread_pool_executor.cpp
  input/buffer_keymap.cpp
  input/input_event.cpp

  ${PROJECT_SOURCE_DIR}/include/common/mir/constexpr_utils.h
)

target_include_directories(mircommon
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/common
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/common
)

target_link_libraries(mircommon
  PUBLIC
    mircommondispatch
    mircommoninput
    mirsharedlogging
    mirsharedlttng
    mirsharedsharedlibrary
    mirsharedthread
    mirevents
    mirtime
)


set_target_properties(mircommon

  PROPERTIES
  SOVERSION ${MIRCOMMON_ABI}
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

install(TARGETS mircommon LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/common/mir
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mircommon"
)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/common/mir_toolkit
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mircommon"
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mircommon.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mircommon.pc
  @ONLY
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mircommon.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
add_custom_target(

    generate-mircommon-symbols-map
    COMMAND ${PROJECT_SOURCE_DIR}/tools/symbols_map_generator/main.py
        --library-name mir_common
        --version ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        --symbols-map-path="${CMAKE_CURRENT_SOURCE_DIR}/symbols.map"
        --include-dirs="$<JOIN:$<TARGET_PROPERTY:mircommon,INTERFACE_INCLUDE_DIRECTORIES>,:>"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

add_custom_target(
    check-mircommon-symbols-map
    COMMAND ${PROJECT_SOURCE_DIR}/tools/symbols_map_generator/main.py
        --library-name mir_common
        --version ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        --symbols-map-path="${CMAKE_CURRENT_SOURCE_DIR}/symbols.map"
        --include-dirs="$<JOIN:$<TARGET_PROPERTY:mircommon,INTERFACE_INCLUDE_DIRECTORIES>,:>"
        --diff
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
