# Preface

This file describes how Mir multiplexes multiple platform displays into one `Display`: initial policy application, composite output identity, configuration apply/rollback, observer fan-out, and cursor selection.

Read the top-level `.kb/agents.md` file before continuing below.


# Concepts

## Initial policy and fan-out

[`MultiplexingDisplay`](../multiplexing_display.cpp) immediately fetches `configuration()`, runs `initial_configuration_policy->apply_to()` and `confirm()`, and calls `configure()` in its constructor. [`the_display_configuration_policy()`](../default_configuration.cpp) defaults to [`CloneDisplayConfigurationPolicy`](../default_display_configuration_policy.cpp) (selecting an opaque pixel format and leaving disconnected or modeless outputs at `mir_power_mode_on`, unlike [`SideBySideDisplayConfigurationPolicy`](../default_display_configuration_policy.cpp) and [`SingleDisplayConfigurationPolicy`](../default_display_configuration_policy.cpp), which set unused outputs to `mir_power_mode_off`).

`register_configuration_change_handler()`, `pause()`, and `resume()` delegate to every component `Display` without deduplication or rollback.

## Composite output identity

[`CompositeDisplayConfiguration`](../multiplexing_display.cpp) synthesizes `DisplayConfigurationOutputId` sequentially from 1, sets `card_id` to the 0-based component index, and rewrites output names via `name_for_output()` as `type-perTypeCount` when `card_id == 0` and `type-card-perTypeCount` when `card_id != 0` (counting 1-based per `(card_id, type)`).

## Mutable output iteration

`UserDisplayConfigurationOutput` exposes mutable references into `CompositeDisplayConfiguration`'s `backing_store` for 13 fields, but mutable `for_each_output()` copies only 11 back into the underlying component `DisplayConfiguration` (`used`, `top_left`, `current_mode_index`, `current_format`, `power_mode`, `orientation`, `scale`, `form_factor`, `gamma`, `custom_logical_size`, `custom_attribute`), omitting `logical_group_id` and `subpixel_arrangement` (so mutations to those two do not reach component configurations or survive `clone()`). Lookups throw `std::out_of_range` for unknown IDs, and `index_for_id()` matches component configurations by pointer identity.

## Apply and rollback

Both `configure()` and [`apply_if_configuration_preserves_display_buffers()`](../multiplexing_display.cpp) `dynamic_cast<CompositeDisplayConfiguration const&>(conf)`. `configure()` applies component configurations sequentially with no rollback on partial failure. `apply_if_configuration_preserves_display_buffers()` stops at the first component returning `false`, does not revert that failing component, reverts prior components in reverse order against a pre-apply snapshot, and throws `IncompleteConfigurationApplied` if any revert returns `false`.

## Observer multiplexer

[`DisplayConfigurationObserverMultiplexer`](../display_configuration_observer_multiplexer.cpp) replays `initial_configuration` on both `register_interest()` and `register_early_observer()` when `current_config` is already cached. Only `initial_configuration()` and `configuration_applied()` update `current_config`; `initial_configuration()` holds `mutex` across observer dispatch whereas `configuration_applied()` copies `current_config` under `mutex` and dispatches unlocked. [`the_display_configuration_observer()`](../default_configuration.cpp) and `the_display_configuration_observer_registrar()` share one cached singleton.

## Cursor selection

[`the_cursor()`](../default_configuration.cpp) picks [`NullCursor`](../null_cursor.h) for `null`, otherwise calls `Display::create_hardware_cursor()` unless `--cursor=software`, falling back to [`SoftwareCursor`](../software_cursor.cpp). Before `wrap_cursor()`, `the_cursor()` calls `show(the_default_cursor_image())`, seeding `current_cursor_image` (required because `SoftwareCursor::scale()` unconditionally calls `show(current_cursor_image)` and dereferences it).

`MultiplexingDisplay::create_hardware_cursor()` requires every component `Display` to return a non-null cursor (catching exceptions or any null result to return `nullptr` globally). [`MultiplexingCursor`](../multiplexing_hw_cursor.cpp) fans `show()`, `hide()`, `move_to()`, and `scale()` across all platform cursors, but delegates `renderable()` and `needs_compositing()` only to `platform_cursors.front()`.

[`SoftwareCursor`](../software_cursor.cpp) throws `std::logic_error` on zero-sized cursor images, uploads pixels into an `argb_8888` software buffer, scales logical size and hotspot by `current_scale`, and shifts renderable position by `old_hotspot - new_hotspot` when hotspot or scale changes.
