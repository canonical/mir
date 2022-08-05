add_library(mirudev OBJECT
  udev_wrapper.cpp
)

target_include_directories(
  mirudev

  PUBLIC
    ${PROJECT_SOURCE_DIR}/src/include/platform
)

target_link_libraries(
  mirudev

  PRIVATE
    ${UDEV_LDFLAGS}
)

list(APPEND MIR_PLATFORM_REFERENCES ${UDEV_LDFLAGS})
set(MIR_PLATFORM_REFERENCES ${MIR_PLATFORM_REFERENCES} PARENT_SCOPE)
