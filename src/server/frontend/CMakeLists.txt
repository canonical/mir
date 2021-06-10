set(
  FRONTEND_SOURCES

  session_credentials.cpp
  basic_mir_client_session.cpp
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/frontend/shell.h
  ${PROJECT_SOURCE_DIR}/src/include/server/mir/frontend/buffer_stream.h
)

add_library(
  mirfrontend OBJECT

  ${FRONTEND_SOURCES}
)

# Fix build race condition - ensure the protobuf header is built before us.
add_dependencies(mirfrontend mirprotobuf)

