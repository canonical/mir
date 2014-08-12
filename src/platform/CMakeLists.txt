include_directories(${PROJECT_SOURCE_DIR}/include/platform)

set(MIRPLATFORM_ABI 1)

add_library(
  mirplatform SHARED
  shared_library_loader.cpp
  $<TARGET_OBJECTS:mirplatformgraphicscommon>
  $<TARGET_OBJECTS:miroptions>
  $<TARGET_OBJECTS:mirfatal>
)

target_link_libraries(mirplatform
  mircommon
  ${EGL_LDFLAGS} ${EGL_LIBRARIES}
  ${GLESv2_LDFLAGS} ${GLESv2_LIBRARIES}
)

set_target_properties(mirplatform
  PROPERTIES
  SOVERSION ${MIRPLATFORM_ABI}
)


install(TARGETS mirplatform LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_subdirectory(graphics/)
add_subdirectory(options)
add_subdirectory(fatal)

set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mirplatform")
set(COMMON_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/mircommon")
set(LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/mirplatform.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/mirplatform.pc
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/mirplatform.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
