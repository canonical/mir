add_library(
    mirnullreport OBJECT

    compositor_report.cpp
    display_report.cpp
    input_report.cpp
    null_report_factory.cpp
    scene_report.cpp
    seat_report.cpp
    shell_report.cpp
    shell_report.h
)

target_include_directories(mirnullreport
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/server
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/server
)

target_link_libraries(mirnullreport
  PUBLIC
    mirplatform
    mircommon
    mircore
)
