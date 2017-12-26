find_package(PIL REQUIRED)

include_directories(
        ${PROJECT_SOURCE_DIR}/include/miral
        ${PROJECT_SOURCE_DIR}/include/client
)

function(png2header png header varname)
    add_custom_command(
        OUTPUT ${header}
        COMMAND python3 ${PROJECT_SOURCE_DIR}/tools/png2header.py ${png} ${varname} > ${header}
        DEPENDS ${png}  ${PROJECT_SOURCE_DIR}/tools/png2header.py
    )
endfunction()

add_subdirectory(spinner)
add_subdirectory(desktop)

png2header(
    ${CMAKE_CURRENT_SOURCE_DIR}/mir-peace-wall.png
    ${CMAKE_CURRENT_SOURCE_DIR}/mir_peace_wall.h
    mir_peace_wall
)

add_custom_target(miral-run ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-run.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-run
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-run
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_custom_target(miral-xrun ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-xrun.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-xrun
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-xrun
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_custom_target(miral-screencast ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-screencast.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-screencast
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-screencast
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_custom_target(miral-desktop ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-desktop.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-desktop
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-desktop
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_custom_target(miral-app ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/miral-app.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-app
)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/miral-app
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_library(miral-shell-lib STATIC
    tiling_window_manager.cpp   tiling_window_manager.h
    floating_window_manager.cpp floating_window_manager.h
    decoration_provider.cpp     decoration_provider.h
    titlebar_config.cpp         titlebar_config.h
    mir_peace_wall.h
)

set_source_files_properties(decoration_provider.cpp PROPERTIES COMPILE_FLAGS
    "${CMAKE_CXX_FLAGS}  -Wno-overlength-strings")

pkg_check_modules(FREETYPE freetype2 REQUIRED)
target_include_directories(miral-shell-lib PRIVATE ${FREETYPE_INCLUDE_DIRS})
target_compile_definitions(miral-shell-lib PRIVATE -DTYPO_SUPPORTS_FREETYPE)
target_link_libraries(miral-shell-lib miral-spinner miral ${FREETYPE_LIBRARIES})

mir_add_wrapped_executable(miral-shell
    shell_main.cpp
)

target_link_libraries(miral-shell
    miral-shell-lib
    miral
)

