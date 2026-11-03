# Preface

This file covers how a seat aggregates input device state: the `SeatInputDeviceTracker`, the `CursorController`, the `CursorObserverMultiplexer`, and the accessibility `InputEventTransformer` that wraps a seat.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Downstream callback locking

`SeatInputDeviceTracker::dispatch` releases `device_state_mutex` before calling `dispatcher->dispatch` and `observer->seat_dispatch_event`. `CursorController` releases `cursor_state_guard` before calling `cursor->show` or `hide` and its downstream observer. Therefore, those callbacks can re-enter and remove themselves.
There are two exceptions to unlocked callbacks. Cursor-motion observers: `update_cursor` calls `cursor_observer->cursor_moved_to` while the tracker still holds `device_state_mutex`. For usability changes, `pointer_usable` and `pointer_unusable` hold `serialize_pointer_usable_unusable` across cursor and drag icon updates and the observer call. Those callbacks must not re-enter either method.

See [`dispatch`](../seat_input_device_tracker.cpp), [`set_cursor_image_locked`](../cursor_controller.cpp) and [`pointer_usable`/`pointer_unusable`](../cursor_controller.cpp).

## §2 - Key events require consistent state

The seat drops a key event from an unknown device. For a known device, it drops an event when its transition is inconsistent with tracked scan codes (down when already held, up when not held).
For known devices, modifier actions bypass the scan-code consistency check.

See [`filter_input_event`](../seat_input_device_tracker.cpp).


# Concepts

## Seat input device tracker

[`SeatInputDeviceTracker`](../seat_input_device_tracker.h) holds one cursor position, the aggregate button state, touch spots, and a `DeviceData` per device.
`update_states` recomputes the seat button state as the bitwise OR of every device's buttons.
`update_cursor` accumulates motion when the pointer event carries no absolute position, and then confines. Cursor coordinates are `float` because the acceleration curve of libinput yields sub-pixel increments.
`confine_function` clamps to the output `input_region` first and then to the `confined_region`. Each clamp is independent, so disjoint regions do not form a general intersection.
`num_pointing_devices` is an atomic refcount: the first `add_pointing_device` and the last `remove_pointing_device` drive `cursor_observer->pointer_usable()`/`pointer_unusable()`.

## Device state synthesis

[`create_device_state`](../seat_input_device_tracker.cpp) emits the currently held scan codes and buttons per device. It pushes `KEY_CAPSLOCK`, `KEY_NUMLOCK`, or `KEY_SCROLLLOCK` twice when that lock is active but its key is not physically held.

## Cursor controller

[`CursorController`](../../../include/server/mir/input/cursor_controller.h) is both a `CursorObserver` and a `DragIconController`.
`set_cursor_image_locked` unlocks `cursor_state_guard` before calling `cursor->show`/`hide` and notifying the downstream observer. A null or zero-sized image counts as hidden (`image_set_to(nullptr)`).
`pointer_usable`/`pointer_unusable` are serialized by the dedicated `serialize_pointer_usable_unusable` mutex rather than the state guard, and only notify the observer on the first transition.
The drag icon surface is moved to the cursor on motion and on `set_drag_icon`, and shown/hidden with pointer usability.

## Cursor observer multiplexer

[`CursorObserverMultiplexer`](../cursor_observer_multiplexer.cpp) records the last position, usability, and image under its own `mutex`.
On `register_interest`/`register_early_observer` it replays that state to the new observer: `pointer_usable` or `pointer_unusable`, then `cursor_moved_to`, then `image_set_to`.

## Accessibility event transformer

[`InputEventTransformer`](../input_event_transformer.cpp) is a `Seat` that forwards to a wrapped seat, and is itself wrapped as a seat.
Its constructor registers a private `StubDevice` with pointer and keyboard capabilities named `"accessibility-device"` through `seat->add_device`.
`transform` copies live transformers out of `input_transformers` under `mutex` before invoking them, because a transformer can remove itself in response to its own dispatched event.
