# Preface

This file describes how Mir multiplexes multiple platform displays into one `Display`: composite output identity, configuration apply/rollback, observer fan-out, and cursor selection.

Read the top-level `.kb/agents.md` file before continuing below.


# Concepts

## Composite output identity

[`CompositeDisplayConfiguration`](../multiplexing_display.cpp) synthesizes output IDs sequentially from 1, sets `card_id` to the component index, and rebuilds names as `type[-card]-perTypeCount`.

## Mutable output iteration

The mutable `for_each_output` copies back only the explicitly enumerated mutable bits (`used`, `top_left`, `current_mode_index`, `current_format`, `power_mode`, `orientation`, `scale`, `form_factor`, `gamma`, `custom_logical_size`, `custom_attribute`). Lookups throw `std::out_of_range` for unknown IDs, and `index_for_id` matches components by pointer identity.

## Apply and rollback

[`apply_if_configuration_preserves_display_buffers()`](../multiplexing_display.cpp) does not revert the sink that failed, reverts prior sinks in reverse order, and throws `IncompleteConfigurationApplied` if any revert fails.

## Observer multiplexer

[`DisplayConfigurationObserverMultiplexer`](../display_configuration_observer_multiplexer.cpp) replays `initial_configuration` on registration when a configuration is already cached. Only `initial_configuration` and `configuration_applied` update `current_config`. [`the_display_configuration_observer()`](../default_configuration.cpp) and the registrar share one cached singleton.

## Cursor selection

[`the_cursor()`](../default_configuration.cpp) picks `NullCursor` for `null`, otherwise a hardware cursor unless the option is `software`, falling back to `SoftwareCursor`. [`SoftwareCursor`](../software_cursor.cpp) scales the hotspot by `current_scale`, and `scale()` recreates the renderable through `show(current_cursor_image)`.
