set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")

include_directories(
  ${CMAKE_SOURCE_DIR}/include/platforms/mesa
  ${PROJECT_SOURCE_DIR}/src/platforms/mesa/include/
)

add_subdirectory(common/)
add_subdirectory(server/)
