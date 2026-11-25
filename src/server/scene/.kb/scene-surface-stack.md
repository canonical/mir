# Preface

This file documents the `SurfaceStack` in the scene. This object models surface depth layers,
insertion order, raise order, and per-compositor occlusion. It backs the scene, input scene,
frontend stack, and session lock.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Depth-layer observer lifetime

- A `SurfaceDepthLayerObserver` holds a raw `SurfaceStack*` and must not outlive the stack it was created for.


# Concepts

## Shared instance

[`default_configuration.cpp`](../default_configuration.cpp) routes `the_scene()`, `the_input_scene()`, `the_frontend_surface_stack()`, and `the_session_lock()` through one `CachedPtr<scene::SurfaceStack>` member (`scene_surface_stack`) declared in [`default_server_configuration.h`](../../../include/server/mir/default_server_configuration.h). `the_surface_stack()` wraps that same object through `wrap_surface_stack()`, which by default returns it unchanged, so all five accessors yield one `SurfaceStack`.

## Depth layers and insertion/raise order

[`SurfaceStack`](../surface_stack.h) stores `surface_layers` indexed by `mir_depth_layer_get_index`, each inner vector ordered bottom to top. `scene_elements_for()` walks them bottom to top, while `surface_at()` walks them in reverse (top to bottom).

`focus_order` is parallel to `surface_layers` but records insertion and raise order, not focus. [`insert_surface_at_top_of_depth_layer()`](../surface_stack.cpp) moves a surface to the back on `add_surface()`, `raise()`, and depth-layer changes. `send_to_back()` and `swap_z_order()` leave it untouched. Focus changes alone do not update it, because shell and window manager focus setters do not raise surfaces. The header comment claiming the front is stale, like the comment in `add_observer()`. `add_observer()` replays the list with `reverse_view`, so observers receive the most recently added or raised surface first.

## Rendering trackers

`rendering_trackers` is keyed by raw `Surface*`. A surface is occluded only when occluded in all active compositors (`occlusions == active_compositors_`). `active_compositors()` prunes occlusions of compositors that are no longer registered. Any query for an unknown `CompositorID` throws `std::logic_error`.

## Depth-layer observer

`SurfaceDepthLayerObserver`'s `depth_layer_set_to()` re-raises the surface, and moves it to the top of its new layer.

## Locking and notifications

`lock()` and `unlock()` use `compare_exchange_strong`, and the system logs and ignores duplicate requests. `raise(SurfaceSet)` copies each layer and compares it before and after. `send_to_back()` scans for a surface that still needs moving.

Both operations emit `surfaces_reordered` only when a layer actually changed. The single-surface `raise()` always notifies when the surface is found, and `remove_surface()` emits `surface_removed` only when the surface was found. While locked, `surface_can_be_shown()` hides every surface whose `visible_on_lock_screen()` is false.
