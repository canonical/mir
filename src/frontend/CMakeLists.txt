set(
  FRONTEND_SOURCES

  application_mediator.cpp
  null_application_mediator_report.cpp
  protobuf_message_processor.cpp
  null_message_processor.cpp

  resource_cache.cpp
  ${PROTO_HDRS}
)

if (MIR_TRANSPORT STREQUAL "socket")
list(APPEND FRONTEND_SOURCES
  protobuf_socket_communicator.cpp
  make_protobuf_socket_communicator.cpp
  socket_session.cpp
)
elseif (MIR_TRANSPORT STREQUAL "binder")
list(APPEND FRONTEND_SOURCES
  protobuf_binder_communicator.cpp
  make_protobuf_binder_communicator.cpp
  binder_service.cpp
)
endif()

if (MIR_PLATFORM STREQUAL "android")
  list(APPEND FRONTEND_SOURCES application_mediator_android.cpp)
elseif (MIR_PLATFORM STREQUAL "gbm")
  include_directories(${DRM_INCLUDE_DIRS})
  list(APPEND FRONTEND_SOURCES application_mediator_gbm.cpp)
endif()

# TODO replace this frig with an RTTI enabled binder library
if (MIR_TRANSPORT STREQUAL "binder")
set_source_files_properties(
  protobuf_binder_communicator.cpp
  binder_service.cpp
  PROPERTIES
  COMPILE_FLAGS "-fno-rtti")
endif()

add_library(
  mirfrontend STATIC

  ${FRONTEND_SOURCES}
)

# Fix build race condition - ensure the protobuf header is built before us.
add_dependencies(mirfrontend mirprotobuf)

target_link_libraries(
  mirfrontend

  mirshell
  mirlogging
  3rd_party
  ${Boost_LIBRARIES}
)
