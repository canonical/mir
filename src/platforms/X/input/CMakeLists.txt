include_directories(
  ${PROJECT_SOURCE_DIR}/include/platform
#  ${CMAKE_SOURCE_DIR}
#  ${PROJECT_SOURCE_DIR}/src/include/platform
#  ${PROJECT_SOURCE_DIR}/src/include/common
#  ${PROJECT_SOURCE_DIR}/src/include/server
#  ${PROJECT_SOURCE_DIR}/src/include/client
#  ${Boost_INCLUDE_DIRS}
#  ${GLESv2_INCLUDE_DIRS}
#  ${CMAKE_SOURCE_DIR}
#  ${UMOCKDEV_INCLUDE_DIRS}
#  ${ANDROID_HEADERS_INCLUDE_DIRS}
)

add_library(
  mirplatforminputX MODULE
  
  X_input.cpp
  )

set_target_properties(
  mirplatforminputX PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/server-modules
  OUTPUT_NAME input-X
  PREFIX ""
  LINK_FLAGS "-Wl,--version-script,${CMAKE_CURRENT_SOURCE_DIR}/symbols-X-input.map"
)

install(TARGETS mirplatforminputX LIBRARY DESTINATION ${MIR_SERVER_PLATFORM_PATH})
