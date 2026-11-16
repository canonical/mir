# Plan: runtime-loadable (out-of-tree) protocol implementations for `wayland_rs`

Status: first increment implemented (see [Progress](#progress)). Based on the `wayland-rs/08-spinner-splash-size` stack (#5234), where the
Wayland frontend runs on `wayland_rs` and `libmirwayland` / the C++ protocol generator are gone.

## Problem

`wayland_rs` can only serve protocols whose XML is in `wayland-protocols/` **at build time**:

- `build_script/main.rs` parses the XML, then generates `protocols.rs` (via `wayland-scanner`),
  `dispatch.rs` (typed `Dispatch`/`GlobalDispatch` impls for `ServerState`), `middleware.rs`,
  `ffi.rs` (cxx bridge), and the C++ abstract classes in `wayland_rs_cpp/`.
- `wayland_server_generated.rs::register_globals` hard-codes one
  `create_global::<ServerState, I, _>` per global, and `mir::wayland::GlobalFactory` (C++) has one
  hard-coded pure virtual `create_<global>()` per global.
- The Rust crate is a `staticlib` with no public headers; nothing outside `mirserver` can name
  its types.

The out-of-tree mechanism (`miral::WaylandExtensions::Builder`, `mir::Server::add_wayland_extension`,
`mir::WaylandExtensionHook`) no longer works on this stack:

- `WaylandExtensionHook::builder` now only receives `run_on_wayland_mainloop`
  (`src/include/server/mir/default_server_configuration.h`).
- `miral::WaylandExtensions::Context::display()` returns `nullptr`
  (`src/miral/wayland_extensions.cpp`, `FrigContext`).
- `mf::get_session(wl_client const*)`, `mf::get_session(wl_resource*)` and
  `mf::get_window(wl_resource*)` are stubs, so `miral::application_for` / `miral::window_for`
  return null.
- The reference out-of-tree example (`tests/miral/example_xdg_shell_v6.cpp`) was removed.

Downstream shells therefore have **no** way to add a protocol. This plan restores that.

Assumption: "out-of-tree protocol implementations" = protocol XML + implementation supplied by a
shell/plugin (C++ via MirAL, optionally Rust/C via `dlopen`) that Mir has never seen at build time.

## Enabling facts (wayland-backend 0.3.15 / wayland-server 0.31.13)

- `wayland_backend::server::Handle::create_global<D>(&'static Interface, u32, Arc<dyn GlobalHandler<D>>)`
  — globals need no Rust type, only an `Interface` descriptor.
- `ObjectData<D>::request(self: Arc<Self>, &Handle, &mut D, ClientId, Message<ObjectId, OwnedFd>) -> Option<Arc<dyn ObjectData<D>>>`
  — untyped request dispatch; returning `ObjectData` initialises a `new_id`.
- `Handle::send_event(Message<ObjectId, RawFd>)`, `post_error`, `destroy_object`, `object_info`,
  `get_object_data` — everything needed for untyped events and lifecycle.
- `Interface` / `MessageDesc` are plain structs of `&'static` slices, so they can be built at
  runtime and `Box::leak`ed (protocols are loaded once per process).
- `protocol::same_interface` compares by pointer **or name** (`protocol.rs:278`), so a runtime
  descriptor that references `"wl_surface"` interoperates with the scanner-generated one.
- These APIs are reachable via `state.handle.backend_handle()` (already used in
  `src/protocol_impls/wl_fixes.rs`) and share `D = ServerState`, so typed (in-tree) and untyped
  (out-of-tree) objects coexist in one `Display`.

### Stability and documentation

- Public, non-hidden API; `#![warn(missing_docs)]` and every method used here has rustdoc,
  including panic conditions.
- Last breaking server-side change was 0.3.0 (2023-09-02, `ObjectData::destroyed` signature);
  0.3.1–0.3.15 are additive/bugfix (`CHANGELOG.md`).
- Building `Interface`s at runtime is not a documented use case (docs assume `wayland-scanner`
  output), so behaviour must be pinned down by tests.

## Options considered

| Option                                                           | Summary                                                                                                                                                    | Verdict                                                                                                                                                                                       |
| ---------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| A. libwayland (`system`) backend + expose `wl_display*`          | `wayland-server` feature `system` gives `Handle::display_ptr()` and `ObjectId::from_ptr`/`as_ptr`, so the existing MirAL `Builder` ABI could keep working. | Rejected as the main path: re-introduces libwayland under Rust, mixes two object models, and contradicts this stack's removal of libwayland types. Kept only as an emergency compat fallback. |
| B. Runtime ("dynamic") protocol layer on the untyped backend API | Parse XML at runtime, build leaked `Interface`s, generic `GlobalHandler`/`ObjectData` forwarding to C++ over a small fixed FFI; typed C++ codegen on top.  | **Recommended.** Pure Rust, no per-protocol Rust code, small stable ABI.                                                                                                                      |
| C. Out-of-tree Rust crates linked as rlib / Rust-ABI `dlopen`    | Unstable Rust ABI; requires rebuilding Mir.                                                                                                                | Rejected. Rust plugins instead use B's C ABI via a thin helper crate.                                                                                                                         |

## How runtime loading works (Option B)

1. **Get the XML** at runtime (file path, embedded string, or plugin) and parse it with the
   same parser as `build_script/protocol_parser.rs`.
1. **Build descriptors.** Map each `<arg type>` to `ArgumentType`, set `since` and
   `is_destructor`. Leak the result to get `&'static Interface`.
   - `arg_interfaces` (object args) are only compared by name, so placeholders and
     self-references are fine.
   - `child_interface` (`new_id`) is stored as the new object's interface and used to parse its
     requests (`rs/server_impl/client.rs:597-680`), so it must be the **full** descriptor.
   - Therefore build children first, in topological order of `new_id` references, and reject
     `new_id` cycles at load time. Building cyclic graphs would need references to
     not-yet-initialised `Interface`s. Object-argument cycles such as `xdg_toplevel.set_parent`
     are unaffected, because they use name-only placeholders.
1. **Register a global:**
   ```rust
   handle.backend_handle().create_global::<ServerState>(
       iface, version, Arc::new(DynamicGlobal { cpp_factory }))
   ```
   `DynamicGlobal: GlobalHandler<ServerState>` — `can_view` applies the existing extension policy
   (`WaylandGlobalFactory::can_view`); `bind` returns a `DynamicObject`.
1. **Dispatch requests.** `DynamicObject: ObjectData<ServerState>::request` gets an untyped
   `Message { sender_id, opcode, args }`. The backend has already validated the signature and
   created any `new_id` child; return `Some(child ObjectData)` to initialise it. Marshal args
   and call C++ `on_request(opcode, args)`. `destroyed()` notifies C++.
1. **Send events** via `handle.send_event(Message { … })`, plus `post_error` and
   `destroy_object`, after validating against the `MessageDesc` and object version.
1. **Threading/timing.** Everything runs on the event-loop thread. While running, use the
   published `DisplayHandle` (as `WaylandServer::create_output_global` does) and return an RAII
   handle that removes the global on drop.

## Design

### Rust side (`wayland_rs/src/dynamic/`)

1. **Shared protocol parser**: move `build_script/protocol_parser.rs` into a small workspace
   crate used by both the build script and runtime, so they agree on XML semantics (enums,
   `since`, `allow-null`, destructors).
1. **`DynamicProtocolRegistry`**: `load_xml(&str) -> Result<ProtocolHandle>`; builds `Interface`
   graphs, resolving cross-protocol references (`wl_surface`, `wl_output`, `wl_seat`, …) to the
   scanner-generated statics first, then other loaded dynamic protocols; rejects name clashes with
   in-tree interfaces and interface-less `new_id`s.
1. **`DynamicGlobal`** and **`DynamicObject`** as described above.
1. **Cross-protocol object interop**: generate (in the existing build script) a lookup table
   `interface name -> fn(ObjectId) -> Option<SharedPtr<C++ object>>` for in-tree interfaces, using
   `Resource::from_id` + `.data::<Arc<Mutex<XxxWrapper>>>()` as `dispatch.rs` does today, so a
   dynamic request carrying a `wl_surface` arrives in C++ as the real `mir::wayland::Surface`.
   The reverse (a dynamic protocol creating an in-tree-typed child via `new_id`) is unsupported
   initially (`wayland_server::ResourceData` has no public constructor in 0.31) — reject at load.
1. **Event/lifecycle API** on a `DynamicResource` box: `send_event(opcode, args)`, `post_error`,
   `version`, `object_id`, `client`, `destroy_and_delete`; event-loop-thread only, with the
   existing `WaylandExecutor` / `drain_queue` for cross-thread work.
1. **Runtime registration**: `WaylandServer::create_dynamic_global(...)` returning an RAII
   `Box<DynamicGlobalHandle>`, modelled on `create_output_global` / `OutputGlobal`, plus a queued
   path for registrations made before `run()` (mirrors `pending_fd_listeners`).

### C++ side

1. **Internal generic interface** (cxx bridge, `wayland_rs_cpp/`): `DynamicGlobalFactory`
   (`bind`, `can_view`) and `DynamicResourceHandler` (`on_request`, `on_destroyed`); no
   per-protocol types cross the bridge.
1. **Public, ABI-stable MirAL API** (pimpl; new symbols in `src/miral/symbols.map` and debian
   symbols, per `.github/instructions/abi-symbols.instructions.md`): register a protocol from XML
   plus a factory via `miral::WaylandExtensions`, and a `miral::WaylandResource`-style value type
   (send_event, post_error, client, application/window lookup). Deprecate `Context::display()`
   and the `wl_client*`/`wl_resource*` overloads of `application_for`/`window_for`, which are
   already non-functional on this stack.
1. **Typed wrapper generator for out-of-tree users**: factor the C++ emitter
   (`build_script/cpp_protocol_generation.rs` / `cpp_builder.rs`) into an installed tool that
   emits header-only typed classes on top of the generic MirAL API, so downstream code looks like
   the in-tree `mir::wayland::*` classes without depending on Mir's internal cxx ABI.
1. **Wiring**: extend `configure_wayland_extensions` / `WaylandExtensions::run_builders`
   (`src/server/frontend_wayland/wayland_default_configuration.cpp`) so dynamic extensions
   participate in `--wayland-extensions`, `--add/drop-wayland-extensions` and
   `conditionally_enable` exactly like built-ins.
1. **Optional `dlopen` plugins** (later): a C entry point
   (`mir_wayland_protocol_plugin_v1(registrar*)`) over the same generic API, loaded from a config
   option; a small Rust helper crate for Rust plugin authors.

## Phased work items

Progress against these items is tracked in [Progress](#progress).

1. Spike: hand-written dynamic global loading an in-repo test XML at runtime, registered via
   `backend_handle().create_global`, bound by a client in the `tests/wayland_rs` harness; prove
   interop with a `wl_surface` argument.
1. Extract shared protocol model crate; implement `DynamicProtocolRegistry` + Rust unit tests
   (descriptor building, cycles, cross-refs, clash/invalid-XML rejection, version/`since`).
1. `DynamicGlobal` / `DynamicObject` + generic cxx bridge; tests in `tests/wayland_rs`.
1. Generated in-tree interop table.
1. Runtime add/remove while running + pre-run queue.
1. Public MirAL API + symbols + docs; wiring into extension options.
1. Typed C++ generator tool + a new reference out-of-tree example (replacing the removed
   `tests/miral/example_xdg_shell_v6.cpp`); miral/WLCS tests.
1. Optional: `dlopen` plugin loader + Rust helper crate.
1. Update `doc/sphinx/` (how-to for shell authors) and release notes for the MirAL deprecations.

## Progress

Done (first increment, `src/dynamic/` + `tests/wayland_rs/test_dynamic_protocol.cpp`):

- Runtime registry (`load_dynamic_protocol(xml)`), reusing `build_script/protocol_parser.rs` via
  `#[path]` rather than a separate crate. Loading is idempotent for identical XML. It rejects
  invalid XML, clashes with in-tree (generated `BUILTIN_INTERFACE_NAMES`) or already-loaded
  interfaces, `new_id`s without an interface or of an in-tree or unknown interface, and `new_id`
  cycles. A failed load registers nothing.
- Generic cxx bridge:
  - `DynamicArgument`/`DynamicArgumentKind` shared types.
  - C++ `DynamicGlobalHandler` (`bind`, `can_view`) and `DynamicResourceHandler` (`on_request`,
    `on_constructor_request`, `on_destroyed`) in `wayland_rs_cpp/include/dynamic_protocol.h`.
  - Rust `DynamicResource` (`send_event`, `post_error`, `destroy`, `create_resource`,
    `set_handler`, `client`, …).
- `WaylandServer::create_dynamic_global` (running server only) returning an RAII
  `DynamicGlobal`.
- Requests are dispatched to C++; object arguments carry the target's handler.
  `mir::wayland::ProtocolError` is posted to the client, and other exceptions become
  `wl_display` implementation errors.
- `send_event` fully validates before reaching the backend (opcode, `since`, arity, kinds,
  nullability, embedded nulls, object/`new_id` ownership and interface, fds), so C++ misuse
  returns an error instead of panicking.
- A null `GlobalFactory` now registers no in-tree globals. The test fixture relies on this.

Not done yet: in-tree interop table (object args of in-tree interfaces arrive with only an id),
pre-run registration queue, MirAL API, typed generator, `dlopen`, docs, dependency caps.

## Risks / open questions

- **Panics abort Mir.** `wayland-backend` panics (not `Err`) on malformed usage
  (`rs/server_impl/client.rs:134-201,657`): wrong event opcode/signature, wrong-interface/null
  object or `new_id` in events, cross-client objects, and — on *incoming requests* — a `new_id`
  whose `MessageDesc` lacks `child_interface`. A panic across the cxx boundary aborts. Reject
  interface-less `new_id` at load time, pre-validate every C++ `send_event`, and fuzz/unit-test
  descriptor building.
- **Dependency pinning.** Workspace `Cargo.toml` has `wayland-backend = ">=0.3.12"` /
  `wayland-server = ">=0.31.13"` with no upper bound; cap to `<0.4` / `<0.32` before relying on
  the untyped API.
- **Validation parity.** The backend validates signatures, but enum values, `allow-null` and
  event `since` must be enforced by our generic layer (generated code does this today).
- **Performance.** Generic marshalling per request (allocations for strings/arrays); measure in
  the spike.
- **Leaked `Interface`s** are never freed — fine for load-once protocols; removing a global does
  not free descriptors.
- **MirAL ABI.** New API must be pimpl'd and versioned; confirm naming and the deprecation plan
  with maintainers.
