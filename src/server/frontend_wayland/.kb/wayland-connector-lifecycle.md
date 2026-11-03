# Preface

This file covers the lifetime, ordering, threading, and client-tracking invariants of [`WaylandConnector`](../wayland_connector.cpp), which owns the `wl_display`, publishes the frontend's globals, and accepts clients.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Global publication order

- `allocator->bind_display(display, executor)` runs in [`WaylandConnector`](../wayland_connector.cpp)'s member initializer list before `wl_display_set_global_filter()`, extension builders, or any globals.
- External `WaylandExtensions::run_builders()` runs before `WlCompositor` and `WlSeat` (GTK3 expects extension builders registered before the seat).
- Built-in globals split around `extensions->init()`: `WlCompositor`, `WlSubcompositor`, `WlFixes`, `WlSeat`, `OutputManager`, and `WlDataDeviceManager` precede `extensions->init()`, while `WlShm`, `WpViewporter`, and conditional `LinuxDRMSyncobjManager` (published iff any `RenderingPlatform` supplies `DRMRenderingProvider`) follow it.

## §2 - Filter and member destruction order

- `extension_filter` is declared before `display` so it outlives `wl_display`, because `wl_global_remove` invokes the global filter during display teardown.
- Reverse member destruction destroys `extensions` before `executor`, `executor` before built-in globals, and built-in globals before `display` (`cleanup_display` calls `wl_display_flush_clients()` and `wl_display_destroy()`).

## §3 - Construction and teardown run off the dispatch thread

- Unlike dispatch-time operations, construction and teardown call Wayland APIs (`wl_display_create`, `wl_display_destroy_clients`, `wl_event_source_remove`, `wl_display_destroy`) on the thread that constructs or destroys the connector, not `dispatch_thread`.


# Concepts

## Construction, sockets, and teardown

- [`WaylandConnector`](../wayland_connector.cpp) creates the `wl_display`, its [`WaylandExecutor`](../wayland_executor.cpp), the globals, the client-created listener ([`WlClient::setup_new_client_handler()`](../wl_client.cpp)), `pause_source`, and the listening socket in its constructor on the constructing thread. `WAYLAND_DISPLAY` selects `wl_display_add_socket` (throwing `std::system_error` on failure) versus `wl_display_add_socket_auto` (`MIR_FATAL_ERROR` on null); with `arw_socket`, `$XDG_RUNTIME_DIR/<wayland_display>` is changed to mode `0666` with `chmod`.
- `start()` runs `wl_display_run()` on `dispatch_thread`. `stop()` signals `pause_signal` (`wl_display_terminate()` on the loop) and joins `dispatch_thread`.
- [`~WaylandConnector()`](../wayland_connector.cpp) calls `wl_display_destroy_clients()` on the destroying thread before halting `dispatch_thread`. While `dispatch_thread` is joinable, it runs `allocator->unbind_display()` on the Wayland thread through a `std::promise` before calling `stop()`, then removes `pause_source`.
- `client_socket_fd()` creates an `AF_LOCAL` `SOCK_STREAM` `socketpair` (without `SOCK_CLOEXEC`) and defers `wl_client_create` onto `executor` (failures there are logged, not propagated; see the `TODO`). The `connect_handler` overload records the callback in `connect_handlers`, which is accessed only on the Wayland event loop and consumed when `WlClient` opens the session.
- `on_surface_created(client, id, callback)` invokes `callback` immediately if `WlSurface::from(resource)` already exists, or queues it in `WlCompositor::surface_callbacks` keyed by `(wl_client*, id)` until `create_surface`.
- The global filter callback resolves the Mir `Session` through `get_session(client)` (`WlClient::from(client).client_session()`, which calls `MIR_FATAL_ERROR` if `client` is missing or expired in `client_map`) and delegates to `extension_filter`.

## Client backends and serials

- Two `Client` implementations exist: libwayland [`WlClient`](../wl_client.cpp) (wired by `WaylandConnector`) and the `wayland_rs` bridge [`WaylandClient`](../wayland_client.cpp) + [`WaylandClientNotifier`](../wayland_client_notifier.cpp). Both record `serial → InputEvent` in a deque capped at 100 entries (`push_front`/`pop_back`).
- [`WlClient`](../wl_client.cpp) registers `wl_display_add_client_created_listener` (`ConstructionCtx`, Standard Layout), destroys unauthorized clients via `wl_client_destroy(client)`, passes a non-owning `Fd{IntOwnedFd{wl_client_get_fd(client)}}` to `shell->open_session()`, and draws serials from `wl_display_next_serial(display)`. It retains a self-`shared_ptr` (`owned_self`) until the `wl_client` destroy listener resets it (`is_being_destroyed() == !owned_self`), closing the Mir session in `~WlClient()` only when the last external `shared_ptr<WlClient>` drops.
- [`WaylandClientNotifier`](../wayland_client_notifier.cpp) callbacks run on the Wayland event-loop thread, hand every [`WaylandClient`](../wayland_client.cpp) a shared `std::atomic<uint32_t>` serial counter (`++(*serial_source)`), pass an owning `::dup(socket_fd)` to `shell->open_session()`, and kill unauthorized or failed-`dup()` clients by posting `wl_display` error code 3 (`implementation`) on object id 1. On `client_removed`, `mark_being_destroyed()` closes the Mir session immediately even if `shared_ptr<WaylandClient>` references outlive the client.
