include_directories(
  ${CMAKE_SOURCE_DIR}/include/platforms/gbm
        ${PROJECT_SOURCE_DIR}/src/platforms/gbm-kms/include/
)

add_subdirectory(server/)
