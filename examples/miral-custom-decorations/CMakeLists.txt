pkg_check_modules(FREETYPE freetype2 REQUIRED IMPORTED_TARGET)

target_link_libraries(exampleserverconfig
  mirserver
  mirwayland
  miral
)

include_directories(
  ${PROJECT_SOURCE_DIR}/include/miral
  ${PROJECT_SOURCE_DIR}/src/include/server
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/wayland
)

mir_add_wrapped_executable(miral-custom-decorations
    miral_custom_decorations.cpp

    input.cpp                          	input.h
    renderer.cpp                       	renderer.h
    user_decoration_example.cpp        	user_decoration_example.h
    user_decoration_manager_example.cpp	user_decoration_manager_example.h
    window.cpp                     	window.h
					threadsafe_access.h
					theme.h
)

target_link_libraries(miral-custom-decorations
    example-shell-lib
    PkgConfig::FREETYPE
)
