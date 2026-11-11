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
- [`enqueue()`](../wayland_executor.cpp) uses double-checked locking on an atomic `ExecutionState` (`Running`/`TerminationRequested`/`Stopped`) to reject work without locking when not running.
- `State::on_wayland_thread` is `thread_local`. When set, `enqueue()` runs the work inline instead of queueing it. The flag is per-thread and shared by all `State` instances, not per-loop. Therefore, `spawn()` from a thread dispatching another executor loop will run inline on the wrong loop. This issue is latent because Mir creates a single `WaylandExecutor` (in [`WaylandConnector`](../wayland_connector.cpp)). After termination, `enqueue()` drops work submitted off the Wayland thread, because `enqueue_termination()` clears the flag only on the destroying thread.
- The `State` constructor queues a work item that sets `on_wayland_thread = true`. The executor constructor then writes the `eventfd`, because the constructor itself cannot notify the loop.

## Termination and destruction

- [`~WaylandExecutor()`](../wayland_executor.cpp) moves its `state` into a cleanup functor and calls `enqueue_termination()`, which pushes it to the queue front, clears `on_wayland_thread`, and sets `TerminationRequested`.
- `EventLoopDestroyedHandler::on_destroyed()` always calls `drain()` first, which runs any pending termination functor. That functor removes the `wl_event_source`, and its destruction releases the executor's moved `state` reference. Then, still holding the queue lock, it removes the source itself only if `work_queue.use_count() != 1`. A count of 1 means the source was already removed, and removing it again will double-free.
- `State::on_notify()` removes the destruction handler once the state leaves `Running`.

## Platform details

- `EventLoopDestroyedHandler` must be Standard Layout for `wl_container_of` (enforced by `static_assert`).
- The notification `eventfd` is created with `EFD_CLOEXEC | EFD_SEMAPHORE | EFD_NONBLOCK`.
