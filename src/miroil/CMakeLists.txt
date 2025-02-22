set(MIROIL_ABI 6)
set(MIROIL_VERSION_MAJOR ${MIROIL_ABI})
set(MIROIL_VERSION_MINOR 0)
set(MIROIL_VERSION_PATCH 0)
set(MIROIL_VERSION ${MIROIL_VERSION_MAJOR}.${MIROIL_VERSION_MINOR}.${MIROIL_VERSION_PATCH})


set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)
set(miroil_include ${PROJECT_SOURCE_DIR}/include/miroil)

add_definitions(-DMIR_LOG_COMPONENT_FALLBACK="miroil" -DMIROIL_ENABLE_DEPRECATIONS=0)

add_library(miroil SHARED
    compositor.cpp ${miroil_include}/miroil/compositor.h    
    edid.cpp ${miroil_include}/miroil/edid.h
    mirbuffer.cpp ${miroil_include}/miroil/mirbuffer.h
    persist_display_config.cpp ${miroil_include}/miroil/persist_display_config.h
    display_configuration_policy.cpp ${miroil_include}/miroil/display_configuration_policy.h
    display_configuration_controller_wrapper.cpp ${miroil_include}/miroil/display_configuration_controller_wrapper.h
    display_listener_wrapper.cpp ${miroil_include}/miroil/display_listener_wrapper.h
    eventdispatch.cpp ${miroil_include}/miroil/eventdispatch.h
    event_builder.cpp ${miroil_include}/miroil/event_builder.h
    input_device.cpp ${miroil_include}/miroil/input_device.h
    input_device_observer.cpp ${miroil_include}/miroil/input_device_observer.h
    mir_server_hooks.cpp ${miroil_include}/miroil/mir_server_hooks.h
    mir_prompt_session.cpp ${miroil_include}/miroil/mir_prompt_session.h
    open_gl_context.cpp ${miroil_include}/miroil/open_gl_context.h    
    prompt_session_listener.cpp ${miroil_include}/miroil/prompt_session_listener.h
    prompt_session_manager.cpp ${miroil_include}/miroil/prompt_session_manager.h    
    set_compositor.cpp ${miroil_include}/miroil/set_compositor.h
    surface.cpp ${miroil_include}/miroil/surface.h
    ${miroil_include}/miroil/display_configuration_storage.h
    ${miroil_include}/miroil/display_id.h
)

target_include_directories(miroil
    PUBLIC  "${miroil_include}"
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include/platform
    ${PROJECT_SOURCE_DIR}/src/include/gl
    ${PROJECT_SOURCE_DIR}/include/renderers/gl
    ${PROJECT_SOURCE_DIR}/include/server
    ${PROJECT_SOURCE_DIR}/include/client
    ${PROJECT_SOURCE_DIR}/include/miral
    ${PROJECT_SOURCE_DIR}/src/include/server
)

target_link_libraries(miroil
    PRIVATE
        mircommon
        mirserver
        miral
)

set_target_properties(miroil
        PROPERTIES
        SOVERSION ${MIROIL_ABI}
        LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
        LINK_DEPENDS ${symbol_map}
)

execute_process(COMMAND uname -m OUTPUT_VARIABLE MIR_ARCH)

find_program(MIR_DPKG_GENSYMBOLS dpkg-gensymbols)
if (CMAKE_COMPILER_IS_GNUCXX   AND  # clang generates slightly different symbols (but we don't care)
    MIR_USE_LD STREQUAL "ld"   AND  # lld on 22.04 generates slightly different symbols (but we don't care)
    MIR_ARCH MATCHES 64        AND  # On 32bit platforms int64_t presents as `long long` (but we've `long`)
    MIR_DPKG_GENSYMBOLS)            # Using dpkg-gensymbols only makes sense on Debian based distros

    if (MIR_DPKG_GENSYMBOLS)
        add_custom_target(check-miroil-symbols ALL
            DEPENDS miroil ${PROJECT_SOURCE_DIR}/debian/libmiroil${MIROIL_ABI}.symbols
            COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/check-and-update-debian-symbols.py ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} miroil ${MIR_VERSION} ${MIROIL_ABI} ${MIROIL_VERSION_MINOR}
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            VERBATIM)
    endif()
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/miroil.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/miroil.pc
    @ONLY
)

if(TARGET doc)
    add_custom_target(regenerate-miroil-symbols-map
            ${CMAKE_CURRENT_SOURCE_DIR}/regenerate-miroil-symbols-map.py ${CMAKE_BINARY_DIR}/doc/xml/*.xml > ${symbol_map}
            DEPENDS doc)
endif()

add_custom_target(
        generate-miroil-symbols-map
        COMMAND ${PROJECT_SOURCE_DIR}/tools/symbols_map_generator/main.py
        --library-name miroil
        --version ${MIROIL_VERSION}
        --symbols-map-path="${CMAKE_CURRENT_SOURCE_DIR}/symbols.map"
        --external-headers-directory="${PROJECT_SOURCE_DIR}/include/miroil"
        --include-dirs "$<JOIN:$<TARGET_PROPERTY:miroil,INCLUDE_DIRECTORIES>,:>"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

add_custom_target(
        check-miroil-symbols-map
        COMMAND ${PROJECT_SOURCE_DIR}/tools/symbols_map_generator/main.py
        --library-name miroil
        --version ${MIROIL_VERSION}
        --symbols-map-path="${CMAKE_CURRENT_SOURCE_DIR}/symbols.map"
        --external-headers-directory="${PROJECT_SOURCE_DIR}/include/miroil"
        --include-dirs "$<JOIN:$<TARGET_PROPERTY:miroil,INCLUDE_DIRECTORIES>,:>"
        --diff
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

install(TARGETS     miroil                           LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY   ${CMAKE_SOURCE_DIR}/include/miroil       DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(FILES       ${CMAKE_CURRENT_BINARY_DIR}/miroil.pc    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
