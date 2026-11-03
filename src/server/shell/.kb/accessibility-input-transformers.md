# Preface

This file covers the accessibility input transformers under `src/server/shell/` and how [`BasicAccessibilityManager`](../basic_accessibility_manager.cpp) registers them into the input pipeline.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Consumption contract

- `transform_input_event` returns `true` to consume the event and `false` to pass it on.
- [`InputEventTransformer::transform()`](../../input/input_event_transformer.cpp) invokes transformers in registration order and stops at the first that returns `true`. Unconsumed events reach the seat.

## §2 - Reentrancy and removal

- A transformer can `remove()` itself while handling an event it dispatched, so `transform()` caches strong references and never holds the registry lock while invoking.
- [`append()`](../../input/input_event_transformer.cpp) stores a `weak_ptr` and silently ignores a duplicate (compared by raw pointer). `remove()` takes a `shared_ptr`.


# Concepts

## Manager and toggles

- [`BasicAccessibilityManager`](../basic_accessibility_manager.cpp) holds each transformer. Its `*_enabled(bool)` setters add or remove transformers from the shared `InputEventTransformer`. All transformers start disabled.
- [`repeat_rate()`](../basic_accessibility_manager.cpp) returns `nullopt` when key repeat is disabled. `repeat_delay()` is independent of that flag.

## Defaults

- Key repeat rate and delay default to 25 and 600 (commented as Weston/Sway parity). Cursor scale is clamped to `[0, 100]`.

## Hover click

- [`BasicHoverClickTransformer`](../basic_hover_click_transformer.cpp): The grace period is `grace_period_percentage` = 0.1 of `hover_duration` (default 1000 ms). The click dispatcher then waits `hover_duration - grace`.
- Cursor motion before the grace alarm cancels and reschedules the initializer, restarting the grace window. When the alarm fires, `hover_click_origin` pins the latest position.
- With an origin pinned and the dispatcher pending, motion `>= cancel_displacement_threshold` (10, compared squared) cancels. Motion `<= reclick_displacement_threshold` (5) is ignored to suppress a reclick.
- Dispatch emits synthetic primary down then up events. `mutable_state` must be declared before `hover_initializer`, which reads it during construction.

## Mousekeys

- [`BasicMouseKeysTransformer`](../basic_mousekeys_transformer.cpp): Motion uses one repeating alarm every 2 ms (500 Hz), created on the first directional keydown and reset once all directions are released.
- Speed is `acceleration_curve.evaluate(t) * dt` (pixels per invocation). The default curve is `{30, 100, 100}` = quadratic 30, linear 100, constant 100.
- Opposite horizontal (or vertical) directions zero that axis. A stopped cursor resets the acceleration start time.
- `max_speed_` defaults to `{400, 400}` and is clamped to `>= 0` per axis with a warning.

## Simulated secondary click

- [`BasicSimulatedSecondaryClickTransformer`](../basic_simulated_secondary_click_transformer.cpp): Consumes a real primary down, then fires a synthetic secondary click after `hold_duration` (1000 ms).
- Motion beyond `displacement_threshold` (20) cancels the hold. The transformer re-dispatches the consumed down event followed by the current motion event.
- A primary up before the alarm cancels the hold and emits a primary click. That up event is consumed.

## Slow keys

- [`BasicSlowKeysTransformer`](../basic_slow_keys_transformer.cpp): Each keydown is buffered in `keys_in_flight` keyed by keysym, with an alarm delayed by `config.delay` (1000 ms).
- On up, an untriggered alarm is canceled and reported rejected (event consumed). An already-triggered alarm lets the up pass through.

## Sticky keys

- [`BasicStickyKeysTransformer`](../basic_sticky_keys_transformer.cpp): Modifier up events are consumed and buffered in `pending`. They flush, after the triggering event, on the next non-modifier up, pointer button-up, or last touch-up.
- With `should_disable_if_two_keys_are_pressed_together`, holding more than one modifier at once temporarily disables the transformer until all are released. The option is checked on every modifier-down while the transformer is not yet disabled, so enabling it mid-hold takes effect on the next modifier press. Once `is_temporarily_disabled` is set, the flag is not re-evaluated until all modifiers are released.
