include_directories(
  ${PROJECT_SOURCE_DIR}/include/common
  ${PROJECT_SOURCE_DIR}/include/client
  )

add_library(mirevdevutilsobjects OBJECT
    input_modifier_utils.cpp
    )

