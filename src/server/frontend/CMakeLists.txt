set(
  FRONTEND_SOURCES

  session_credentials.cpp
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/frontend/buffer_stream.h
)

add_library(
  mirfrontend OBJECT

  ${FRONTEND_SOURCES}
)

