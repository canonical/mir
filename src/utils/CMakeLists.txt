set(CMAKE_C_FLAGS
  "${CMAKE_C_FLAGS} -std=c99 -Wall -fno-strict-aliasing -Wextra"
)

include_directories(
  ${PROJECT_SOURCE_DIR}/include/client
)

# Note that production binaries don't have underscores in their names
mir_add_wrapped_executable(mirout out.c)
target_link_libraries(mirout mirclient m)

if (MIR_EGL_SUPPORTED)
mir_add_wrapped_executable(mirvanity vanity.c)
target_link_libraries(mirvanity eglapp)
endif()

mir_add_wrapped_executable(mirin in.cpp)
target_link_libraries(mirin mirclient)

mir_add_wrapped_executable(mirrun run.cpp)
target_link_libraries(mirrun mircommon ${Boost_LIBRARIES} )

mir_add_wrapped_executable(mirscreencast screencast.cpp)
target_link_libraries(mirscreencast
  mirclient
  ${EGL_LIBRARIES}
  ${GLESv2_LIBRARIES}
)

add_custom_target(mirbacklight ALL
  cp ${CMAKE_CURRENT_SOURCE_DIR}/backlight.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mirbacklight
)
install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mirbacklight
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)
