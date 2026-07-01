---
applyTo: src/server/frontend_wayland/**,src/wayland/**,wayland-protocols/**
description: Integrating and implementing Wayland protocols correctly in Mir.
---

# Wayland protocol integration

When integrating a new Wayland protocol:

1. **Place protocol XML** in the `wayland-protocols/` directory.

1. **Generate wrapper code** using the `mir_generate_protocol_wrapper()` CMake function (defined in `cmake/MirCommon.cmake`):

   ```cmake
   # In the appropriate CMakeLists.txt (usually src/wayland/CMakeLists.txt)
   mir_generate_protocol_wrapper(mirwayland "zwp_" path/to/protocol.xml)
   ```

   This generates a `*_wrapper.h` (class declarations) and `*_wrapper.cpp` (thunks, message handling) from the XML.

1. **Implement the protocol** in `src/server/frontend_wayland/`:

   - Create implementation files in the `mir::frontend` namespace
   - Inherit from the generated `wayland::ProtocolName::Global` (for bindable globals) or `wayland::ProtocolName` (for resources)
   - Override the pure virtual request methods generated from the protocol XML
   - Follow existing patterns (see `foreign_toplevel_manager_v1.cpp`, `ext_image_capture_v1.cpp`, etc.)

1. **Create a factory function** to instantiate the protocol global:

   - Expose a `create_*_manager()` or `create_*_global()` function
   - Pass required dependencies via `WaylandConnector::Context` (e.g., `MainLoop`, `Executor`, `Shell`)
   - Return `std::shared_ptr<wayland::ProtocolName::Global>`

1. **Integrate with WaylandConnector**:

   - If the protocol needs existing server components (MainLoop, Shell, etc.), access them through `WaylandConnector::Context`
   - If the protocol requires a new server component, add it to `DefaultServerConfiguration` and expose via `Server`
   - Register the protocol global with the wayland display in the appropriate initialization code

1. **Hook into MirAL** (if needed): integrate with `miral::WaylandExtensions` in your shell to enable/disable the protocol.

## Wayland object lifecycle

The generated code manages object lifecycle automatically:

- **`wayland::Resource`** is the base class for all Wayland protocol objects. It wraps a `wl_resource*` and sets up a destruction thunk via `wl_resource_set_implementation()`.
- **`wayland::Global`** is the base class for bindable globals (advertised to all clients). It wraps `wl_global*` and handles version negotiation in the bind callback.
- **`wayland::LifetimeTracker`** provides `destroyed_flag()` and `add_destroy_listener()` for safe cross-thread destruction tracking.
- **`wayland::Weak<T>`** is Mir's alternative to `std::weak_ptr` for Wayland objects. Use it for any reference to a Wayland resource — it checks a `destroyed_flag`, as the objects are owned by the Wayland infrastructure and can be destroyed at any time.
- **Version-gated events**: Generated code provides `version_supports_*()` and `send_*_event_if_supported()` methods. Always use the `_if_supported` variant when the event was added in a later protocol version.
- **Destruction is automatic**: When the `wl_resource` is destroyed (by client or server), the generated `resource_destroyed_thunk` deletes the C++ wrapper object.

## Protocol correctness

- **Input validation**: Validate all client-supplied values at the protocol boundary. Reject invalid inputs (e.g., non-positive dimensions, invalid enums) using `mw::ProtocolError` with the appropriate error code.
- **Error code matching**: The error code must belong to the *interface of the resource* receiving the error. Sending an error code from a different interface will be misinterpreted by clients (e.g., [PR #4890](https://github.com/canonical/mir/pull/4890) — layer-shell error sent on a layer-surface resource).
- **Error reporting**: Throw `ProtocolError` via `BOOST_THROW_EXCEPTION(mw::ProtocolError(...))` or `throw wayland::ProtocolError{...}`. The generated request thunks catch this and call `wl_resource_post_error()` automatically — do not call `wl_resource_post_error()` directly.
- **Lifecycle management**: Wayland objects can be destroyed by the client at any time — use `wayland::Weak<T>`.
- **Fix all protocol instances**: When fixing a bug in a protocol handler, check all other protocol implementations for the same bug. Copy-paste is common across xdg-shell stable/v6 and mir_shell implementations (e.g., [PR #4869](https://github.com/canonical/mir/pull/4869)).
- **Configure sequences**: The frontend may receive state updates piecemeal (e.g., "fullscreen", "new size", "activated" separately). Be aware of how configure events batch these changes — see [issue #4700](https://github.com/canonical/mir/issues/4700).
- **Protocol spec compliance**: Always reference the actual Wayland protocol specification for behavioral requirements (e.g., DnD action selection semantics, positioner constraint logic).
