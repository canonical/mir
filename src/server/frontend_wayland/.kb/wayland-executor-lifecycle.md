# Preface

This file covers the lifetime and threading invariants of [`WaylandExecutor`](../wayland_executor.cpp), the `mir::Executor` that marshals work onto a Wayland event loop.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Wayland thread affinity

- While the loop is dispatching, touch Wayland objects only on its `wl_event_loop` thread. Marshal such work through [`WaylandExecutor::spawn()`](../wayland_executor.cpp). Connector construction and teardown are the exception (see wayland-connector-lifecycle.md§3).

## §2 - Shared queue ownership

- The `State` work queue must stay `shared_ptr`-owned by both the executor and the loop destruction handler. Do not reduce it to a single owner.


# Concepts

## Queue and state

- [`State`](../wayland_executor.cpp) holds the work queue and is `shared_ptr`-shared between [`WaylandExecutor`](../wayland_executor.h) and the `EventLoopDestroyedHandler` registered on the loop. The `wl_event_source` stores only a raw `State*`.
- [`enqueue()`](../wayland_executor.cpp) checks `State::on_wayland_thread` first (running work inline when `true`), and otherwise uses double-checked locking on atomic `ExecutionState` (`Running`/`TerminationRequested`/`Stopped`) to reject off-thread work once state leaves `Running`.
- `State::on_wayland_thread` is a static `thread_local` shared by all `State` instances rather than per-loop, so `spawn()` from a thread dispatching another executor loop runs inline on the wrong loop (latent because Mir creates a single `WaylandExecutor` in [`WaylandConnector`](../wayland_connector.cpp)). Because `enqueue_termination()` and `drain()` clear `on_wayland_thread` only on the thread executing them, and `enqueue()` checks `on_wayland_thread` before `state`, any other thread whose `on_wayland_thread` remains `true` continues running `enqueue()` work inline even after termination.
- The `State` constructor queues an initial work item setting `on_wayland_thread = true`. The `WaylandExecutor` constructor then writes the `eventfd`, because `State` construction itself cannot notify the loop.
- [`WaylandExecutor::spawn()`](../wayland_executor.cpp) writes `1` to `notify_fd` (`EFD_SEMAPHORE`) unconditionally after `enqueue()`, even when `enqueue()` ran inline on the Wayland thread or dropped work after termination. Conversely, `State::on_notify()` consumes only one semaphore unit via `eventfd_read` per wakeup while draining the entire `workqueue`, leaving surplus semaphore counts that cause benign empty `on_notify()` wakeups.
- `State::on_notify()` catches all exceptions (`catch (...)`) from queued items and logs them at `critical` severity, whereas inline execution via `spawn()` on the Wayland thread propagates exceptions directly to the caller (skipping the `eventfd_write`).

## Termination and destruction

- [`~WaylandExecutor()`](../wayland_executor.cpp) moves its `state` into a cleanup functor and calls `enqueue_termination()`, which pushes it to the queue front, clears `on_wayland_thread` on the destroying thread, and sets `TerminationRequested`.
- `EventLoopDestroyedHandler::on_destroyed()` calls `drain()` first, which runs only `workqueue.front()` if `state == ExecutionState::TerminationRequested` (the termination functor, which removes the `wl_event_source` and releases the executor's moved `state` reference), clears `on_wayland_thread`, sets `Stopped`, and discards all remaining queued functors via `workqueue.clear()`. Still holding the queue lock, `on_destroyed()` removes the source itself only if `work_queue.use_count() != 1`: a count of 1 means the termination functor already ran and removed the source, avoiding a double-free.
- `State::on_notify()` calls `remove_destruction_handler_for_loop()` once `state` leaves `Running` (note: that helper deletes the `EventLoopDestroyedHandler` immediately before `wl_list_remove(&listener->link)`).

## Platform details

- `EventLoopDestroyedHandler` must be Standard Layout for `wl_container_of` (enforced by `static_assert`).
- The notification `eventfd` is created with `EFD_CLOEXEC | EFD_SEMAPHORE | EFD_NONBLOCK`.
