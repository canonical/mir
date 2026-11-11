# Preface

This file documents the scene's idle, text-input, and clipboard hubs: their notification state machines, locking, and the invariants that keep observers consistent.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Alarm callback locking

- The idle alarm callback must run with the `Synchronised<State>` lock held. `AlarmCallback::lock()` acquires it, and `operator()` aborts if invoked unlocked.


# Concepts

## Idle hub state

[`BasicIdleHub`](../basic_idle_hub.h) keeps `first_timeout` equal to `timeouts.begin()->first` (or `nullopt` when empty) so `poke()` avoids a map lookup. `alarm_timeout` is a duration after `poke_time`, or `nullopt` when no alarm is scheduled. `idle_multiplexers` holds multiplexers already notified idle that need `active()` on the next poke.

## Idle alarm and poke

The alarm callback is a `LockableCallback` whose `lock()` takes the `Synchronised<State>` lock, so `alarm_fired()` runs with state locked. It tolerates a `nullopt` `alarm_timeout` (the cancel race) by returning early.

`poke()` is a no-op while inhibition is enabled and a wake lock is held. `inhibit_idle()` returns the existing lock if one is already held.

## Idle multiplexer transitions

`Multiplexer::is_active` starts `true` and only notifies on transitions: `idle()` fires only when active, `active()` only when idle.

## Text input hub

[`BasicTextInputHub`](../basic_text_input_hub.cpp) increments the serial on every `set_handler_state()`. Switching handler forces `new_input_field = true`. The initial-state replay in `send_initial_state()` always passes `new_input_field = true`. `deactivate_handler()` no-ops when the handler does not match the active one.

## Clipboard

[`BasicClipboard`](../basic_clipboard.cpp) releases `paste_mutex` before notifying observers, so a concurrent `paste_source()` can differ from the source passed to `paste_source_set()`. `do_with_paste_source()` runs the callback on the calling thread with the source pinned under the mutex.
