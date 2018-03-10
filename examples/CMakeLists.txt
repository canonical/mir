include_directories(
  ${PROJECT_SOURCE_DIR}/include/miral
  ${PROJECT_SOURCE_DIR}/include/client
)

add_subdirectory(miral-server-lib)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/miral-server-lib
)

add_subdirectory(miral-shell)
add_subdirectory(miral-kiosk)
add_subdirectory(mirclient)
