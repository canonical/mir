add_definitions(-DMIR_LOG_COMPONENT_FALLBACK="mirplatform")

include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/src/include/platform
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)

add_definitions(-DMIR_SERVER_PLATFORM_PATH="${MIR_SERVER_PLATFORM_PATH}")
add_definitions(-DMIR_SERVER_GRAPHICS_PLATFORM_VERSION="${MIR_SERVER_GRAPHICS_PLATFORM_VERSION}")

set(MIR_PLATFORM_OBJECTS
  $<TARGET_OBJECTS:mirplatformgraphicscommon>
  $<TARGET_OBJECTS:miroptions>
  $<TARGET_OBJECTS:mirudev>
)

set(MIR_PLATFORM_REFERENCES
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GL_LDFLAGS} ${GL_LIBRARIES}
  mirwayland
)

function(add_object_library_to_target TARGET OBJECT_LIB)
    get_target_property(HEADERS ${OBJECT_LIB} INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(LIBS ${OBJECT_LIB} INTERFACE_LINK_LIBRARIES)

    if (HEADERS)
        target_include_directories(${TARGET} PUBLIC ${HEADERS})
    endif()

    if (LIBS)
        target_link_libraries(${TARGET} INTERFACE ${LIBS})
    endif()

    target_sources(${TARGET} PRIVATE $<TARGET_OBJECTS:${OBJECT_LIB}>)
endfunction()

function(add_object_libraries_to_target TARGET)
    foreach(OBJECT_LIB ${ARGN})
        add_object_library_to_target(${TARGET} ${OBJECT_LIB})
    endforeach()
endfunction()

add_subdirectory(graphics/)
add_subdirectory(options)
add_subdirectory(udev)

set(MIR_PLATFORM_OBJECTS ${MIR_PLATFORM_OBJECTS} PARENT_SCOPE)
set(MIR_PLATFORM_REFERENCES ${MIR_PLATFORM_REFERENCES} PARENT_SCOPE)

add_library(mirplatform SHARED
  ${PROJECT_SOURCE_DIR}/include/platform/mir/input/input_sink.h
  ${PROJECT_SOURCE_DIR}/include/platform/mir/console_services.h
  ${PROJECT_SOURCE_DIR}/include/platform/mir/executor.h
)

add_object_libraries_to_target(
  mirplatform

  mirplatformgraphicscommon
  miroptions
  mirudev
)

target_link_libraries(mirplatform
  PRIVATE
    mircommon
    ${MIR_PLATFORM_REFERENCES}
)

set_target_properties(
  mirplatform

  PROPERTIES
  SOVERSION ${MIRPLATFORM_ABI}
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

install(TARGETS mirplatform LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirplatform.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirplatform.pc
  @ONLY
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mirplatform.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
