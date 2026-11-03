# Preface

This file covers the surface lifecycle and input handling in the Wayland frontend.
It explains surface roles, commit ordering, explicit synchronization, frame callbacks,
input regions, pointer, axis, and seat behavior, and protocol state machines.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - One role per surface

- A `wl_surface` can take at most one role type for its whole lifetime. [`clear_role()`](../wl_surface.cpp) resets `role` to `null_role` but leaves `previous_role_type` intact. Therefore, [`can_set_role_type()`](../wl_surface.cpp) admits only the same type.
- Re-assigning the same role type is allowed, but a different type throws.

## §2 - Subsurface order before role commit

- [`complete_commit()`](../wl_surface.cpp) applies `pending_surface_order` (z-order only) before `role->commit()`.
- Z-order lives on the parent, outside `WlSurfaceState`, so it takes effect on the parent's next `wl_surface.commit` even when the parent is itself a synchronized subsurface.
- Position behaves differently. [`WlSubsurface::set_position()`](../wl_subcompositor.cpp) writes the child `pending.offset`, so it applies only through the child commit. The commit applies immediately if desynchronized, or from cached state when the parent state next applies. A `set_position` with no later child commit never applies (this deviates from [`wl_subsurface.set_position`](../../../../wayland-protocols/wayland.xml)).


# Concepts

## Commit and surface state

- `WlSurfaceState::buffer` is an `std::optional<wayland::Weak<...>>`. A value of `nullopt` leaves the current buffer unchanged, and a null `Weak` clears it.
- [`commit()`](../wl_surface.cpp) drops a pending offset or input shape equal to the current value before applying.
- A commit can have frame callbacks and no new attachment (`state.buffer == nullopt`, even if a buffer is already current). The commit captures the callbacks into `heartbeat_quirk_frame_callbacks` and flushes them after a delay on `frame_callback_executor` (Firefox heartbeat, issue #1967). It does not flush immediately or on a global 60 Hz timer. [`FrameExecutor`](../frame_executor.cpp) batches them behind a 16 ms alarm. An explicit null attach instead unmaps and sends all pending `frame_callbacks` immediately when the state is applied.

## Explicit synchronization

- With linux-drm-syncobj, a commit carrying an acquire syncpoint defers the whole commit. [`commit()`](../wl_surface.cpp) registers the acquire eventfd with `wl_event_loop_add_fd` and calls `complete_commit()` only from the handler. That handler runs on the Wayland event loop, so no locking is needed.
- The system checks errors in order when either syncpoint is set. [`claim_timeline()`](../linux_drm_syncobj.cpp) raises `no_acquire_point` or `no_release_point` for a missing point, even with no buffer (unlike the [spec's `no_buffer`](../../../../wayland-protocols/linux-drm-syncobj-v1.xml)). Only with both points set does a missing or null buffer raise `no_buffer`. Foreign buffers (`ext_foreign_buffer_v1`) then raise `unsupported_buffer`. A buffer committed with neither point set bypasses these checks even on a syncobj surface.

## Input shape

- An explicitly empty input region is encoded by pushing a zero-size rect into the accumulator, because Mir treats an empty vector as "use the default input shape". An absent input_shape means "use the surface rect".

## Pointer and axis

- `current_buttons` survives leave and enter events. While any button is pressed, the pointer stays pinned to the current surface and will not leave it.
- `set_cursor` requires `serial == enter_serial`. Re-assigning the cursor role is allowed, but a different non-cursor role is a protocol error.
- `BufferCursorImage` copies the mapped buffer with `memcpy`. The system does not retain the mapping lifetime.
- `value120` suppresses `discrete`. If either is sent, the server must also send a non-value120 `axis` event (even with value 0) because it carries the timestamp. `axis_source` is sent only when some axis event was sent.

## Seat

- Capabilities are hardcoded to pointer|keyboard|touch (TODO).
- `ListenerList` is keyed by `void*` client and held by `shared_ptr`, so devices can unregister themselves after destruction.
- `ConsumedKeyTracker` consumes the matching key-up/repeats once a key-down was consumed by an input trigger.

## Screencopy

- Damage `Area`s are matched by `FrameParams` equality, whose `operator==` deliberately excludes `overlay_cursor`.
- The change notifier is created lazily on the first `copy_with_damage`. The area list is cleared past 100 entries.
- The `inverted` flag is never sent because the renderer inverts during `set_output_transform`. The target must be a shm argb8888 buffer with exact size and stride. `copy` and `copy_with_damage` run once only.

## Decorations and dialogs

- `ToplevelsWithDecorations` is shared across all manager bindings. Therefore, rebinding cannot bypass `already_constructed` or lose a retained mode. Entries outlive decoration objects, so the committed mode survives destroy and recreate with no intervening commit (v2).
- An orphaned decoration is a protocol error but is currently only logged (issue #3452).
- `set_modal` records `pending_type`, and `unset_modal` restores only if the type is still `dialog`. `ToplevelsWithDialogs` is per-manager, not shared.

## Activation

- Key-down invalidates all pending tokens. A focus change invalidates tokens not from the newly focused session. An unfocus event invalidates all tokens.
- Non-focused sessions receive a bogus token. `pending_tokens` is guarded by a mutex on the Wayland thread and main loop.
- `activate` enqueues on the main loop and revokes the token. Token data can be null for externally issued tokens.

## Viewporter and output

- Viewport source coordinates are specified after buffer transform and scale, but [`resolve_viewport()`](../wp_viewporter.cpp) only multiplies by scale. Buffer transform is not yet applied. Therefore, cropping, `out_of_buffer` checks, and the inferred size can be wrong for non-identity transforms. An all-(-1) source or (-1,-1) destination unsets the viewport. `associate_viewport` rejects a surface that already has a current or pending viewport.
- `xdg_output` captures `geometry_scale` per-client at creation. Logical pos and size equal extents times scale. Version 3 sends `wl_output.done` (the constructor for initial state, the caller for updates), while version < 3 sends `xdg_output.done`. The server sends `name` once.
