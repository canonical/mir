include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/src/include/server
)

set(MIRAL_ABI 4)
set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)
set(miral_include ${PROJECT_SOURCE_DIR}/include/miral)

pkg_check_modules(YAML REQUIRED yaml-cpp)
pkg_check_modules(GIO REQUIRED gio-2.0 gio-unix-2.0)

add_library(mirclientcpp INTERFACE)

add_definitions(-DMIR_LOG_COMPONENT_FALLBACK="miral" -DMIRAL_ENABLE_DEPRECATIONS=0)

add_library(miral-internal STATIC
    active_outputs.cpp                  active_outputs.h
    application_info_internal.cpp       application_info_internal.h
    basic_window_manager.cpp            basic_window_manager.h window_manager_tools_implementation.h
    coordinate_translator.cpp           coordinate_translator.h
    display_configuration_listeners.cpp display_configuration_listeners.h
    launch_app.cpp                      launch_app.h
    mru_window_list.cpp                 mru_window_list.h
    open_desktop_entry.cpp              open_desktop_entry.h
    static_display_config.cpp           static_display_config.h
    window_info_internal.cpp            window_info_internal.h
    window_management_trace.cpp         window_management_trace.h
    xcursor_loader.cpp                  xcursor_loader.h
    xcursor.c                           xcursor.h
                                        join_client_threads.h
                                        window_info_defaults.h
)

# Already implied by the linker's symbol version script, but can avoid accidents
set_target_properties(miral-internal
    PROPERTIES COMPILE_FLAGS "${CMAKE_CXXFLAGS}  -fvisibility=hidden")

set_source_files_properties(xcursor.c PROPERTIES COMPILE_DEFINITIONS _GNU_SOURCE)

add_library(miral SHARED
    add_init_callback.cpp               ${miral_include}/miral/add_init_callback.h
    application.cpp                     ${miral_include}/miral/application.h
    application_authorizer.cpp          ${miral_include}/miral/application_authorizer.h
    application_info.cpp                ${miral_include}/miral/application_info.h
    canonical_window_manager.cpp        ${miral_include}/miral/canonical_window_manager.h
    command_line_option.cpp             ${miral_include}/miral/command_line_option.h
    cursor_theme.cpp                    ${miral_include}/miral/cursor_theme.h
    display_configuration.cpp           ${miral_include}/miral/display_configuration.h
    external_client.cpp                 ${miral_include}/miral/external_client.h
    keymap.cpp                          ${miral_include}/miral/keymap.h
    minimal_window_manager.cpp          ${miral_include}/miral/minimal_window_manager.h
    runner.cpp                          ${miral_include}/miral/runner.h
    display_configuration_option.cpp    ${miral_include}/miral/display_configuration_option.h
    output.cpp                          ${miral_include}/miral/output.h
    append_event_filter.cpp             ${miral_include}/miral/append_event_filter.h
    wayland_extensions.cpp              ${miral_include}/miral/wayland_extensions.h
    window.cpp                          ${miral_include}/miral/window.h
    window_info.cpp                     ${miral_include}/miral/window_info.h
    window_management_options.cpp       ${miral_include}/miral/window_management_options.h
    window_specification.cpp            ${miral_include}/miral/window_specification.h
    internal_client.cpp                 ${miral_include}/miral/internal_client.h
    set_command_line_handler.cpp        ${miral_include}/miral/set_command_line_handler.h
    set_terminator.cpp                  ${miral_include}/miral/set_terminator.h
    set_window_management_policy.cpp    ${miral_include}/miral/set_window_management_policy.h
    toolkit_event.cpp                   ${miral_include}/miral/toolkit_event.h
    window_management_policy.cpp        ${miral_include}/miral/window_management_policy.h
    window_manager_tools.cpp            ${miral_include}/miral/window_manager_tools.h
                                        ${miral_include}/miral/lambda_as_function.h
    x11_support.cpp                     ${miral_include}/miral/x11_support.h
    zone.cpp                            ${miral_include}/miral/zone.h
)

#include <mir/udev/wrapper.h>
set_source_files_properties(keymap.cpp PROPERTIES COMPILE_FLAGS "-I${PROJECT_SOURCE_DIR}/src/include/platform")

check_cxx_compiler_flag(-Wno-attribute-alias HAS_W_NO_ATTRIBUTE_ALIAS)
if(HAS_W_NO_ATTRIBUTE_ALIAS)
    set_source_files_properties(
            canonical_window_manager.cpp
            internal_client.cpp
            set_window_management_policy.cpp
            window_info.cpp
            window_management_policy.cpp
            window_manager_tools.cpp
            PROPERTIES COMPILE_FLAGS "${CMAKE_CXXFLAGS} -Wno-attribute-alias")
endif()

target_include_directories(mirclientcpp
    INTERFACE "${miral_include}" ${MIRCLIENT_INCLUDE_DIRS}
)

target_include_directories(miral-internal
    PRIVATE "${miral_include}" ${MIRCLIENT_INCLUDE_DIRS}
    PRIVATE ${MIRSERVER_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS}
)

target_link_libraries(miral-internal
    PRIVATE
        mirserver
        ${WAYLAND_CLIENT_LDFLAGS} ${WAYLAND_CLIENT_LIBRARIES}
        ${YAML_LIBRARIES}
        ${GIO_LIBRARIES}
)

target_include_directories(miral
    PUBLIC  "${miral_include}" ${MIRCLIENT_INCLUDE_DIRS}
    PRIVATE ${MIRSERVER_INCLUDE_DIRS}
)

target_link_libraries(miral
    PUBLIC
        mircommon
    PRIVATE
        mirclient
        miral-internal
        mirserver
)

set_target_properties(miral
        PROPERTIES
        SOVERSION ${MIRAL_ABI}
        LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
        LINK_DEPENDS ${symbol_map}
)



if (CMAKE_COMPILER_IS_GNUCXX   AND  # clang generates slightly different symbols (but we don't care)
    MIR_LINK_TIME_OPTIMIZATION AND  # g++ generates slightly different symbols without LTO (but we don't care)
    EXISTS /etc/debian_version)     # Using dpkg-gensymbols only makes sense on Debian based distros

    find_program(MIR_DPKG_GENSYMBOLS dpkg-gensymbols)
    if (MIR_DPKG_GENSYMBOLS)
        add_custom_target(check-miral-symbols ALL
            DEPENDS miral ${PROJECT_SOURCE_DIR}/debian/libmiral${MIRAL_ABI}.symbols
            COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/check-and-update-debian-symbols.py ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} miral ${MIRAL_VERSION} ${MIRAL_ABI}
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            VERBATIM)
    endif()
endif()

set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/miral.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/miral.pc
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${miral_include}/miral/version.h
)

if(TARGET doc)
    add_custom_target(regenerate-miral-symbols-map
            ${CMAKE_CURRENT_SOURCE_DIR}/regenerate-miral-symbols-map.py ${CMAKE_BINARY_DIR}/doc/xml/*.xml > ${symbol_map}
            DEPENDS doc)
endif()

install(TARGETS     miral                           LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")
install(DIRECTORY   ${CMAKE_SOURCE_DIR}/include/miral       DESTINATION "${INCLUDEDIR}")
install(FILES       ${CMAKE_CURRENT_BINARY_DIR}/miral.pc    DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")
