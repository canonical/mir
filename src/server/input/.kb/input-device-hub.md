# Preface

This file describes the input device hub. It covers device registration, observer notification, and configuration persistence across device removal and re-addition.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Locking

The hub mutex is recursive so `add_observer()` can emit the initial device list while holding it. `RegisteredDevice` does not guarantee its own thread safety. Callers must not invoke its non-const methods concurrently.

## §2 - Observer removal

[`ExternalInputDeviceHub::remove_observer()`](../default_input_device_hub.cpp) queues the removal on the observer queue (the main loop by default) and blocks until execution completes. Do not call it from a callback on that loop: the removal cannot run until the callback returns, so the call deadlocks. When the loop is not running, removal runs inline. It does not cancel an `add_observer()` call still queued, so the observer can still be added and notified once the loop runs. Only off-loop calls on a running loop guarantee that no notification arrives after the function returns.


# Concepts

## Device registration

[`DefaultInputDeviceHub::add_device()`](../default_input_device_hub.cpp) matches existing devices by pointer identity (`device_matches`), throws if the device is already managed, and otherwise calls `restore_or_create_device()`.

Device ids come from `create_new_device_id()`, which returns `++device_id_generator`. The generator starts at 0, so the first id is 1.

[`remove_device()`](../default_input_device_hub.cpp) stores the device configuration, removes it from the seat, stops it, and removes its handle.

## Configuration across replug

`store_device_config()` pushes `dev.config()` into the `stored_devices` vector. `get_stored_device_config()` matches by `unique_id()` and erases the matching entry through `erase_if`.

`restore_or_create_device()` reuses the stored configuration when the `unique_id` matches. Otherwise it allocates a new id. This is how per-device settings survive a replug.

## Observer notification

[`DefaultInputDeviceHub::add_observer()`](../default_input_device_hub.cpp) enqueues on `device_queue`, sends `device_added` for every current handle plus `changes_complete`, and then adds the observer. All of these steps run under the hub lock.

`add_device_handle()` and `remove_device_handle()` notify observers synchronously while holding the recursive mutex. The first device added triggers `ready_for_user_input()`. Removing the last device triggers `stop_receiving_input()`.

[`ExternalInputDeviceHub`](../default_input_device_hub.h) marshals notifications onto a `ServerActionQueue`. Its `Internal` observer buffers added/changed/removed devices and, on `changes_complete()`, enqueues one batch to all observers.

## Change transactions

[`for_each_mutable_input_device()`](../default_input_device_hub.cpp) opens `pending_changes` if no transaction is in progress, copies the handle list, unlocks, invokes the callback, then calls `complete_transaction()`.

`device_changed()` appends to `pending_changes` when a transaction is open. Otherwise it notifies observers immediately. It ignores devices no longer in `handles` (debug log only).

`complete_transaction()` moves `pending_changes` out, resets it, and notifies observers of each changed device followed by `changes_complete()`.
