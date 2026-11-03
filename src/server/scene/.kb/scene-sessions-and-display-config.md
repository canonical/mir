# Preface

This file covers the scene-layer session model ([`SessionManager`](../session_manager.cpp), [`SessionContainer`](../session_container.cpp), [`ApplicationSession`](../application_session.cpp)) and the display-configuration path ([`MediatingDisplayChanger`](../mediating_display_changer.cpp), [`OutputPropertiesCache`](../output_properties_cache.cpp)), plus the change-notification observers that feed the compositor.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Session container re-entrancy

- `SessionContainer::for_each` holds its non-recursive `guard` while invoking the callback, so the callback must not call back into the container (`insert_session`, `remove_session`, or `for_each`) or it will deadlock.


# Concepts

## Session lifecycle and shutdown

- [`SessionManager::~SessionManager()`](../session_manager.cpp) snapshots every session through `app_container->for_each` into a local vector, and calls `close_session` on each. The comment notes that this manual close breaks the cyclic strong references between `msh::Session` and `mi::*` implementations.
- `close_session` notifies the event sink, then `stopping` observers, then removes the session from the container, in that order.
- `open_session` constructs an `ApplicationSession`, inserts it into the container, then fires `starting` observers.

## Session container ordering

- [`SessionContainer`](../session_container.cpp) keeps sessions in insertion order in `apps`. `successor_of` and `predecessor_of` wrap around: the successor of the last is the first, and the predecessor of the first is the last.
- For a null session, `successor_of(nullptr)` returns `apps.back()` and `predecessor_of(nullptr)` returns `apps.front()`.
- `successor_of`/`predecessor_of` read `apps` without taking `guard`, unlike `insert_session`/`remove_session`/`for_each`.

## Application session surfaces and streams

- [`ApplicationSession::has_buffer_stream`](../application_session.cpp) reads the `streams` set without `surfaces_and_streams_mutex`, unlike the other surface/stream accessors.
- `default_content_map` is keyed and valued by `weak_ptr`. `create_surface` records only the first stream (`params.streams.value()[0]`) as the surface default content.
- `surface_after` skips window types that cannot take focus (`gloss`, `tip`, `decoration`, and unknown types) and wraps around the surface list.

## Display configuration mediation

- [`MediatingDisplayChanger::configure(conf)`](../mediating_display_changer.cpp) coalesces requests. If `pending_configuration` is already set, an action is in flight. It replaces the pending configuration and returns. Otherwise it enqueues a server action.
- `apply_config` stops and restarts the compositor when new outputs are enabled or the buffer-preserving apply fails, and also when only orientation/scale changed (workaround for #2807, since `Compositor` has no damage-scheduling API).
- On failure, `apply_config` reports `configuration_failed`, reapplies the previous configuration, reports `catastrophic_configuration_error`, and rethrows. The inner handler catches the rethrow, so the catastrophic notification fires on every failure, not only when the revert itself fails.
- Per-session configurations are applied only while that session is focused. `configure(session, conf)` stores the configuration and returns early unless the session is focused. `focus_change_handler` applies a stored configuration on focus.
- `base_configuration_applied` tracks whether the base configuration is live. It is set to false after a successful `apply_config` and true in `apply_base_config`.
- `alarm_factory` is stored but never used.

## Output properties cache

- [`OutputPropertiesCache::properties_for`](../output_properties_cache.cpp) returns an aliasing `shared_ptr` that shares ownership of the cached vector, so the returned properties stay valid. It picks the greatest-scale output whose extents overlap the query.
- DPI is computed from the diagonal of the physical size and the current mode resolution.

## Scene and surface change notifications

- [`SurfaceChangeNotification`](../surface_change_notification.cpp) overrides only a subset of surface callbacks to trigger a scene change: content resize, move, hide, alpha, transform, reception mode, and rename. `frame_posted` reports damage instead.
- `frame_posted` translates surface-local damage to global coordinates using `top_left`, guarded by `mutex` and updated in `moved_to`.
- [`SceneChangeNotification`](../scene_change_notification.cpp) wraps each surface's notifier with a `was_visible` latch, so a hide (visible to hidden) still triggers a scene change.
