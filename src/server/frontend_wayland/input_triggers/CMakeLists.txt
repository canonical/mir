# Input trigger protocol implementation
add_library(mirfrontend-wayland-input-trigger OBJECT
  input_trigger_registration_v1.cpp input_trigger_registration_v1.h
  input_trigger_action_v1.cpp       input_trigger_action_v1.h
  input_trigger_data.cpp            input_trigger_data.h
)

# Override the parent's fallback component with our own
# Use COMPILE_OPTIONS to undefine first, then redefine
target_compile_options(mirfrontend-wayland-input-trigger
  PRIVATE
    -UMIR_LOG_COMPONENT_FALLBACK # Undefine the definition we get from mirserver
    -DMIR_LOG_COMPONENT_FALLBACK="input-triggers"
)

target_include_directories(mirfrontend-wayland-input-trigger
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/include/server
)

target_link_libraries(mirfrontend-wayland-input-trigger
  PUBLIC
    mirplatform
)
