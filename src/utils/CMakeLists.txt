set(CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} -std=c99 -Wall -fno-strict-aliasing -Wextra"
)

include_directories(
  ${PROJECT_SOURCE_DIR}/include/client
)

# Note that production binaries don't have underscores in their names
add_executable(mirping ping.c)
target_link_libraries(mirping mirclient)

add_executable(mirout out.c)
target_link_libraries(mirout mirclient)

install(TARGETS
  mirping
  mirout
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
