find_package(XKBCOMMON REQUIRED)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/src/include/cookie
  ${DRM_INCLUDE_DIRS}
)

add_compile_definitions(MIR_LOG_COMPONENT_FALLBACK="mirclient")

set(MIR_CLIENT_SOURCES)
set(MIR_CLIENT_REFERENCES mircommon mircore)

add_subdirectory(events)
add_subdirectory(input)

add_library(mirclient STATIC
  event.cpp
  event_printer.cpp
  mir_cookie.cpp
  mir_cursor_api.cpp
  ${MIR_CLIENT_SOURCES}
  $<TARGET_OBJECTS:mirsharedinput>
)

set(LINKS
  mircommon

  ${XKBCOMMON_LIBRARIES}
)

target_link_libraries(mirclient
  PUBLIC
    mircore
    ${MIR_CLIENT_REFERENCES}
    ${LINKS}
)
