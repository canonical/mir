set(CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} -std=c99 -Wall -fno-strict-aliasing -Wextra"
)

include_directories(
  ${PROJECT_SOURCE_DIR}/include/client
)

# Note that production binaries don't have underscores in their names
mir_add_wrapped_executable(mirout out.c)
target_link_libraries(mirout mirclient m)

mir_add_wrapped_executable(mirin in.cpp)
target_link_libraries(mirin mirclient)

mir_add_wrapped_executable(mirscreencast screencast.cpp)
target_link_libraries(mirscreencast
  mirclient
  ${EGL_LIBRARIES}
  ${GLESv2_LIBRARIES}
)
