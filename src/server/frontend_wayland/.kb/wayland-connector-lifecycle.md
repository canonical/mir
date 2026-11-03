# Preface

This file covers the lifetime, ordering, and threading invariants of [`WaylandConnector`](../wayland_connector.cpp), which owns the `wl_display`, publishes the frontend's globals, and accepts clients.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Global publication order

- `wl_compositor` is published before the subsequent built-in globals.
- External extension builders run earlier still, before both the compositor and the seat (GTK3 expects the builders before the seat).

## §2 - Filter lifetime

- `extension_filter` must outlive the `wl_display`, because the global filter runs during `wl_global_remove`.

## §3 - Construction and teardown run off the dispatch thread

- Unlike dispatch-time operations, construction and teardown call Wayland APIs (`wl_display_create`, `wl_display_destroy_clients`, `wl_event_source_remove`) on the thread that constructs or destroys the connector, not the dispatch thread.


# Concepts

## Construction and teardown

- [`WaylandConnector`](../wayland_connector.cpp) creates the `wl_display`, its [`WaylandExecutor`](../wayland_executor.cpp), the globals, and the listening socket in one constructor (on the constructing thread). `start()` runs `wl_display_run()` on `dispatch_thread`. `stop()` signals `pause_signal` and joins.
- [`~WaylandConnector()`](../wayland_connector.cpp) destroys clients first. While the dispatch thread is joinable, it runs `unbind_display()` on the Wayland thread through a promise before calling `stop()`. It then removes `pause_source`.
- The global filter callback resolves the session from the client and delegates to `extension_filter`. `extension_filter` is declared before `display`, so it is destroyed last.
- The system accesses `connect_handlers` only on the event loop (see the header comment on the member).

## Client sockets

- `client_socket_fd()` creates a `socketpair` and defers `wl_client_create` onto the executor. Failures there are logged, not propagated (see the `TODO`). The `connect_handler` overload records the handler in `connect_handlers` on the event loop.
- With `arw_socket`, the system changes the listening socket mode to `0666` with `chmod`.

## Serials

- [`WaylandClient`](../wayland_client.cpp) draws serials from a `shared_ptr<std::atomic<uint32_t>>` handed to every client by [`WaylandClientNotifier`](../wayland_client_notifier.cpp), which mirrors libwayland's display-global `wl_display_next_serial`.
- `next_serial()` stores `serial → event` in a deque capped at 100 entries (`push_front`/`pop_back`).

## Client notification

- [`WaylandClientNotifier`](../wayland_client_notifier.cpp) callbacks run on the Wayland event-loop thread.
- On authorizer rejection it kills the client by posting a `wl_display` error on object id 1 with the `implementation` code 3.
- Rust owns the client socket, so the Mir session receives a `dup()` of it. A failed `dup()` also kills the client.
