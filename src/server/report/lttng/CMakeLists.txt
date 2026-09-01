add_library(mirlttng OBJECT

    compositor_report.cpp
    display_report.cpp
    input_report.cpp
    lttng_report_factory.cpp
    scene_report.cpp
    server_tracepoint_provider.cpp
    shared_library_prober_report.cpp
)

target_include_directories(mirlttng
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include/server
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/server
)

target_link_libraries(mirlttng
  PUBLIC
    mirplatform
    mircommon
    mircore
  PRIVATE
    PkgConfig::LTTNG_UST
)

add_library(mirserverlttng SHARED tracepoints.c)

target_include_directories(mirserverlttng
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(
  mirserverlttng

  PkgConfig::LTTNG_UST
)

install(TARGETS mirserverlttng
  LIBRARY DESTINATION ${MIR_TRACEPOINT_LIB_INSTALL_DIR}
)
