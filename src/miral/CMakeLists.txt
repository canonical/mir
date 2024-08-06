include_directories(
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/include/renderer
)

set(MIRAL_ABI 7)
set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)
set(miral_include ${PROJECT_SOURCE_DIR}/include/miral)

pkg_check_modules(YAML REQUIRED IMPORTED_TARGET yaml-cpp)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0 gio-unix-2.0)

add_compile_definitions(MIR_LOG_COMPONENT_FALLBACK="miral" MIRAL_ENABLE_DEPRECATIONS=0)

add_library(miral-internal STATIC
    active_outputs.cpp                   active_outputs.h
    application_info_internal.cpp        application_info_internal.h
    application_selector.cpp             application_selector.h
    basic_window_manager.cpp             basic_window_manager.h window_manager_tools_implementation.h
    system_compositor_window_manager.cpp system_compositor_window_manager.h
    display_configuration_listeners.cpp  display_configuration_listeners.h
    fd_manager.cpp                       fd_manager.h
    input_device_config.cpp              input_device_config.h
    launch_app.cpp                       launch_app.h
    mru_window_list.cpp                  mru_window_list.h
    open_desktop_entry.cpp               open_desktop_entry.h
    static_display_config.cpp            static_display_config.h
    window_info_internal.cpp             window_info_internal.h
    window_management_trace.cpp          window_management_trace.h
    xcursor_loader.cpp                   xcursor_loader.h
    xcursor.c                            xcursor.h
                                         join_client_threads.h
                                         window_info_defaults.h
    window_specification_internal.cpp    window_specification_internal.h
)

# Already implied by the linker's symbol version script, but can avoid accidents
set_target_properties(miral-internal
    PROPERTIES COMPILE_FLAGS "${CMAKE_CXXFLAGS}  -fvisibility=hidden")

set_source_files_properties(xcursor.c PROPERTIES COMPILE_DEFINITIONS _GNU_SOURCE)

check_cxx_compiler_flag(-Wstringop-truncation HAS_W_STRINGOP_TRUNCATION)
if (HAS_W_STRINGOP_TRUNCATION)
    set_source_files_properties(xcursor.c PROPERTIES
        COMPILE_FLAGS "-Wno-error=stringop-truncation"
    )
endif()

add_library(miral-external OBJECT
    add_init_callback.cpp               ${miral_include}/miral/add_init_callback.h
    application.cpp                     ${miral_include}/miral/application.h
    add_init_callback.cpp               ${miral_include}/miral/add_init_callback.h
    application.cpp                     ${miral_include}/miral/application.h
    application_authorizer.cpp          ${miral_include}/miral/application_authorizer.h
    application_info.cpp                ${miral_include}/miral/application_info.h
    canonical_window_manager.cpp        ${miral_include}/miral/canonical_window_manager.h
    config_file.cpp                     ${miral_include}/miral/config_file.h
    configuration_option.cpp            ${miral_include}/miral/configuration_option.h
    ${miral_include}/miral/command_line_option.h
    cursor_theme.cpp                    ${miral_include}/miral/cursor_theme.h
    custom_renderer.cpp                 ${miral_include}/miral/custom_renderer.h
    display_configuration.cpp           ${miral_include}/miral/display_configuration.h
    external_client.cpp                 ${miral_include}/miral/external_client.h
    idle_listener.cpp                   ${miral_include}/miral/idle_listener.h
    input_configuration.cpp             ${miral_include}/miral/input_configuration.h
    keymap.cpp                          ${miral_include}/miral/keymap.h
    minimal_window_manager.cpp          ${miral_include}/miral/minimal_window_manager.h
    display_configuration_option.cpp    ${miral_include}/miral/display_configuration_option.h
    output.cpp                          ${miral_include}/miral/output.h
    append_event_filter.cpp             ${miral_include}/miral/append_event_filter.h
    wayland_extensions.cpp              ${miral_include}/miral/wayland_extensions.h
    window.cpp                          ${miral_include}/miral/window.h
    window_info.cpp                     ${miral_include}/miral/window_info.h
    window_management_options.cpp       ${miral_include}/miral/window_management_options.h
    window_specification.cpp            ${miral_include}/miral/window_specification.h
    internal_client.cpp                 ${miral_include}/miral/internal_client.h
    prepend_event_filter.cpp            ${miral_include}/miral/prepend_event_filter.h
    runner.cpp                          ${miral_include}/miral/runner.h
    session_lock_listener.cpp           ${miral_include}/miral/session_lock_listener.h
    set_command_line_handler.cpp        ${miral_include}/miral/set_command_line_handler.h
    set_terminator.cpp                  ${miral_include}/miral/set_terminator.h
    set_window_management_policy.cpp    ${miral_include}/miral/set_window_management_policy.h
    toolkit_event.cpp                   ${miral_include}/miral/toolkit_event.h
    window_management_policy.cpp        ${miral_include}/miral/window_management_policy.h
    window_manager_tools.cpp            ${miral_include}/miral/window_manager_tools.h
    ${miral_include}/miral/lambda_as_function.h
    x11_support.cpp                     ${miral_include}/miral/x11_support.h
    zone.cpp                            ${miral_include}/miral/zone.h
    decorations.cpp                     ${miral_include}/miral/decorations.h
)

add_library(miral SHARED
    ${miral_include}/miral/runner.h # Syntactically, we need something here
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

target_include_directories(miral-internal
    PRIVATE "${miral_include}"
)

target_link_libraries(miral-internal
    PRIVATE
        mirserver
        PkgConfig::WAYLAND_CLIENT
        PkgConfig::YAML
        PkgConfig::GIO
)

target_include_directories(miral-external
    PUBLIC  "${miral_include}"
)

target_include_directories(miral
    PUBLIC  "${miral_include}"
)

target_link_libraries(miral-external
    PUBLIC
        mircommon
    PRIVATE
        miral-internal
        mirserver
)

target_link_libraries(miral
    PUBLIC
        miral-external
    PRIVATE
        miral-internal
)

set_target_properties(miral
        PROPERTIES
        SOVERSION ${MIRAL_ABI}
        LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
        LINK_DEPENDS ${symbol_map}
)

find_program(MIR_DPKG_GENSYMBOLS dpkg-gensymbols)
if (CMAKE_COMPILER_IS_GNUCXX   AND  # clang generates slightly different symbols (but we don't care)
    MIR_USE_LD EQUAL "ld"      AND  # lld on 22.04 generates slightly different symbols (but we don't care)
    MIR_DPKG_GENSYMBOLS)

    set(MIR_CHECK_MIRAL_SYMBOLS_DEFAULT ALL)
endif()

add_custom_target(regenerate-miral-debian-symbols ${MIR_CHECK_MIRAL_SYMBOLS_DEFAULT}
    DEPENDS miral ${PROJECT_SOURCE_DIR}/debian/libmiral${MIRAL_ABI}.symbols
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/check-and-update-debian-symbols.py ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} miral ${MIRAL_VERSION} ${MIRAL_ABI}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    VERBATIM)

add_custom_target(
        generate-miral-symbols-map
        COMMAND ${PROJECT_SOURCE_DIR}/tools/symbols_map_generator/main.py
            --library-name miral
            --version ${MIRAL_VERSION}
            --symbols-map-path="${CMAKE_CURRENT_SOURCE_DIR}/symbols.map"
            --external-headers-directory="${PROJECT_SOURCE_DIR}/include/miral"
            --include-dirs "$<JOIN:$<TARGET_PROPERTY:miral,INCLUDE_DIRECTORIES>,:>"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

add_custom_target(
        check-miral-symbols-map
        COMMAND ${PROJECT_SOURCE_DIR}/tools/symbols_map_generator/main.py
        --library-name miral
        --version ${MIRAL_VERSION}
        --symbols-map-path="${CMAKE_CURRENT_SOURCE_DIR}/symbols.map"
        --external-headers-directory="${PROJECT_SOURCE_DIR}/include/miral"
        --include-dirs "$<JOIN:$<TARGET_PROPERTY:miral,INCLUDE_DIRECTORIES>,:>"
        --diff
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/miral.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/miral.pc
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${miral_include}/miral/version.h
)

install(TARGETS     miral                           LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY   ${CMAKE_SOURCE_DIR}/include/miral       DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(FILES       ${CMAKE_CURRENT_BINARY_DIR}/miral.pc    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
