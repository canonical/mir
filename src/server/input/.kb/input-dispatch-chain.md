# Preface

This file describes how input events flow from platform devices through the dispatcher chain to the scene. It explains the invariants that each dispatcher enforces. Read it before changing event routing, key repeat, filters, or input timestamps.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Fixed chain order

Events always traverse `KeyRepeatDispatcher` → `KeyboardResyncDispatcher` → `IdlePokingDispatcher` → `EventFilterChainDispatcher` → `SurfaceInputDispatcher`. Each wrapper forwards to its `next_dispatcher`.

## §2 - Device removal reaches key repeat

`KeyRepeatDispatcher` must be registered with the device hub, or removed devices keep repeating keys forever (lp:1675357).


# Concepts

## Chain construction

[`the_input_dispatcher()`](../default_configuration.cpp) builds the chain inside-out: `IdlePokingDispatcher` wraps the event filter chain, `KeyboardResyncDispatcher` wraps that, and `KeyRepeatDispatcher` wraps the resync dispatcher. `EventFilterChainDispatcher` wraps `the_surface_input_dispatcher()`.

`start()` and `stop()` forward down the chain. [`EventFilterChainDispatcher`](../event_filter_chain_dispatcher.cpp) also implements `CompositeEventFilter`. Filters are held as `weak_ptr` references, and expired entries are pruned during `handle()`. A filter that returns `true` consumes the event.

## Key repeat

[`KeyRepeatDispatcher`](../key_repeat_dispatcher.cpp) uses a 500 ms timeout before the first repeat and a 50 ms delay between repeats (set in [`default_configuration.cpp`](../default_configuration.cpp)).

- Meta or modifier keys (`is_meta_key`) never auto-repeat. Pressing one cancels any pending repeat for that device.
- Repeat state is per device in `repeat_state_by_device`. `set_alarm_for_device()` swaps the alarm under `repeat_state_mutex` and resets the old alarm outside the lock.
- `remove_device()` erases the device's state (the alarm destructor cancels it) and clears `touch_button_device` if it matches.
- The device named `"mtk-tpd"` is hardcoded as the touch-button device. The system checks it only when `disable_repeat_on_touchscreen` is true, which the default configuration passes as `false`.
- [`the_default_input_device_hub()`](../default_configuration.cpp) calls `set_input_device_hub()` so removals reach the dispatcher (lp:1675357).

## Idle poking and keyboard resync

[`IdlePokingDispatcher`](../idle_poking_dispatcher.cpp) forwards the event first, then calls `idle_hub->poke()` for key, pointer, and touch events (not resync events).

[`KeyboardResyncDispatcher`](../keyboard_resync_dispatcher.cpp) forwards `start()`, then dispatches a `MirKeyboardResyncEvent` down the chain.

## Filters and options

The VT filter is installed only when the `vt_switching` option is set. If the VT switcher cannot be created, the chain starts with no filters.

Touchspots are disabled by default and enabled only when the `MIR_SERVER_ENABLE_TOUCHSPOTS` option is set.

## Timestamp calibration

[`DefaultEventBuilder::calibrate_timestamp()`](../default_event_builder.cpp) trusts a source timestamp only if it is not in the future and within the last 10 s of `clock->now()` (assuming `CLOCK_MONOTONIC`). Otherwise it substitutes `now`.

## Input manager threading

[`DefaultInputManager`](../default_input_manager.cpp) runs platform `start()` and `stop()` on the `"Mir/Input Reader"` `ThreadedDispatcher` through an `ActionQueue`. State transitions use a CAS state machine (`stopped`/`starting`/`started`/`stopping`) with `on_unwind` guards. The guards check state because of the GCC bug 62258 `uncaught_exception()` workaround.
