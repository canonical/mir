find_package(PIL REQUIRED)
pkg_check_modules(FREETYPE freetype2 REQUIRED)

function(png2header png header varname)
    add_custom_command(
        OUTPUT ${header}
        COMMAND python3 ${PROJECT_SOURCE_DIR}/tools/png2header.py ${png} ${varname} > ${header}
        DEPENDS ${png}  ${PROJECT_SOURCE_DIR}/tools/png2header.py
    )
endfunction()

png2header(
    ${CMAKE_CURRENT_SOURCE_DIR}/mir-peace-wall.png
    ${CMAKE_CURRENT_SOURCE_DIR}/mir_peace_wall.h
    mir_peace_wall
)

add_library(example-shell-lib STATIC
    tiling_window_manager.cpp   tiling_window_manager.h
    floating_window_manager.cpp floating_window_manager.h
    decoration_provider.cpp     decoration_provider.h
    titlebar_config.cpp         titlebar_config.h
    sw_splash.cpp               sw_splash.h
    mir_peace_wall.h
)

set_source_files_properties(decoration_provider.cpp PROPERTIES COMPILE_FLAGS
    "${CMAKE_CXX_FLAGS}  -Wno-overlength-strings")

target_include_directories(example-shell-lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/include/client
    PRIVATE
        ${FREETYPE_INCLUDE_DIRS}
)
target_link_libraries(example-shell-lib PUBLIC miral mirclient PRIVATE ${FREETYPE_LIBRARIES})
