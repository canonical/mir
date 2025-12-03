add_subdirectory(logging)
add_subdirectory(lttng)
add_subdirectory(null)

add_library(
    mirreport OBJECT
    default_server_configuration.cpp
    reports.cpp
    reports.h
)

target_include_directories(mirreport
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include/server
    ${PROJECT_SOURCE_DIR}/src/include/server
)

target_link_libraries(mirreport
  PUBLIC
    mirplatform
    mircommon
    mircore
)
