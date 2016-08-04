pkg_check_modules(NETTLE REQUIRED nettle)

set(PREFIX "${CMAKE_INSTALL_PREFIX}")
set(EXEC_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mircookie.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mircookie.pc
  @ONLY
)

include_directories(
  ${PROJECT_SOURCE_DIR}/src/include/cookie
  ${PROJECT_SOURCE_DIR}/include/cookie
  ${NETTLE_INCLUDE_DIRS}
)

set(MIRCOOKIE_ABI 2)
set(symbol_map ${CMAKE_SOURCE_DIR}/src/cookie/symbols.map)

add_library(mircookie SHARED
  authority.cpp
  const_memcmp.cpp
  hmac_cookie.cpp
)

set_target_properties(mircookie
  PROPERTIES
  SOVERSION ${MIRCOOKIE_ABI}
  LINK_FLAGS "-Wl,--exclude-libs=ALL -Wl,--version-script,${symbol_map}"
  LINK_DEPENDS ${symbol_map}
)

target_link_libraries(mircookie
  ${NETTLE_LDFLAGS} ${NETTLE_LIBS}
)

install(
  TARGETS mircookie
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/cookie/mir
  DESTINATION "include/mircookie"
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mircookie.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
