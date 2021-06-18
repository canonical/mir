find_package(XKBCOMMON REQUIRED)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include/platform
  ${PROJECT_SOURCE_DIR}/include/client
  ${PROJECT_SOURCE_DIR}/src/include/client
  ${PROJECT_SOURCE_DIR}/src/include/cookie
  ${DRM_INCLUDE_DIRS}
)

set(MIRCLIENT_ABI 11)
set(symbol_map ${CMAKE_SOURCE_DIR}/src/client/symbols.map)

add_definitions(-DMIR_LOG_COMPONENT_FALLBACK="mirclient")

set(MIR_CLIENT_SOURCES)
set(MIR_CLIENT_REFERENCES mircommon)

add_subdirectory(events)
add_subdirectory(input)

add_library(mirclientobjects OBJECT

  event.cpp
  event_printer.cpp
  mir_cookie.cpp
  mir_cursor_api.cpp
  ${MIR_CLIENT_SOURCES}
)

add_library(mirclient SHARED $<TARGET_OBJECTS:mirclientobjects> $<TARGET_OBJECTS:mirsharedinput>)

set_target_properties(mirclient

  PROPERTIES
  SOVERSION ${MIRCLIENT_ABI}
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

set(LINKS
  mircommon
  mircookie

  ${XKBCOMMON_LIBRARIES}
)

target_link_libraries(mirclient
  LINK_PRIVATE
  LINK_PUBLIC
  ${MIR_CLIENT_REFERENCES}
  ${LINKS}
)

install(
  TARGETS mirclient
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
