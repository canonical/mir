# Preface

This file covers surface lifecycle, window roles, input translation, clipboard and text-input bridges, capture, and protocol state machines in the Wayland frontend.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - One role per surface

- A `wl_surface` can take at most one role type for its whole lifetime. [`clear_role()`](../wl_surface.cpp) resets `role` to `null_role` while leaving `previous_role_type` intact, so [`can_set_role_type()`](../wl_surface.cpp) admits only the original role type.
- [`set_role()`](../wl_surface.cpp) throws `std::runtime_error` (not `wayland::ProtocolError`) on role-type mismatch and does not check `has_role()`; callers must guard `has_role()` and `can_set_role<T>()` to raise protocol-defined role errors and avoid overwriting an active same-type role.

## §2 - Subsurface order before role commit

- [`complete_commit()`](../wl_surface.cpp) applies `pending_surface_order` (z-order only) before `role->commit()`. Z-order lives on the parent outside `WlSurfaceState`, so it takes effect on the parent's next `wl_surface.commit` even when the parent is a synchronized subsurface. Reordering alone does not call `invalidate_surface_data()`, so a parent commit with no other refresh flag updates Wayland hit-testing (`children`) without pushing updated stream order to `scene::Surface`; also [`remove_subsurface()`](../wl_surface.cpp) erases from `children` without pruning `pending_surface_order`.
- Position and sync mode behave differently: [`WlSubsurface::set_position()`](../wl_subcompositor.cpp) writes the child's `pending.offset`, applying only after a subsequent child commit (and parent apply when synchronized), unlike [`wl_subsurface.set_position`](../../../../wayland-protocols/wayland.xml). `set_desync()` clears `synchronized_` without flushing `cached_state`, `parent_has_committed()` stops recursion at an intermediate synchronized subsurface lacking `cached_state`, and `total_offset()` adds only the immediate parent's `offset()`.


# Concepts

## Commit and surface state

- `WlSurfaceState::buffer` is `std::optional<wayland::Weak<ResourceLifetimeTracker>>`. `nullopt` leaves buffer state unchanged. An empty `Weak` (from `attach(NULL)` or from attaching a `wl_buffer` and destroying it before `commit`) clears `buffer_size_` and `current_foreign_buffer` and flushes `frame_callbacks`, without clearing `WlSurface::current_buffer` — so a commit with `needs_buffer_submission` set (scale, viewport, or transform change) resubmits the stale `current_buffer`. When replacing a live `current_foreign_buffer` with a non-foreign `wl_buffer`, `WlSurface::commit(state)` resolves `ExtForeignBufferV1::from(weak_buffer.value())` (`nullptr`) instead of unsubscribing from `current_foreign_buffer`.
- [`commit()`](../wl_surface.cpp) drops `pending.offset` or `pending.input_shape` equal to the currently applied `offset_` or `input_shape` (ignoring unapplied `WlSubsurface::cached_state`), and `WlSurfaceState::update_from()` merges neither `opaque_region` nor `surface_spec` on cached subsurface commits.
- Any commit with `state.buffer == nullopt` and non-empty `state.frame_callbacks` captures those callbacks into `heartbeat_quirk_frame_callbacks` via [`FrameExecutor`](../frame_executor.cpp) (even if `needs_buffer_submission` resubmits `current_buffer`), batching them behind a one-shot 16 ms `MainLoop` alarm that posts back to `wayland_executor` (Firefox heartbeat, issue #1967). An explicit null attach instead flushes `this->frame_callbacks` immediately and unmaps window roles via `apply_mapped_state()`.

## Explicit synchronization

- With `linux-drm-syncobj-v1`, an acquire syncpoint defers `complete_commit()` by registering the acquire eventfd with `wl_event_loop_add_fd` on the Wayland loop. Deferral does not snapshot `this->pending`: `handle_buffer_ready` consumes `surf->pending` when the fence signals, folding post-commit uncommitted state into the fenced commit. A second commit before signal removes `buffer_ready_source` and immediately signals the superseded `release` point; [`~WlSurface()`](../wl_surface.cpp) does not remove a live `buffer_ready_source`.
- Error precedence when `timeline_set()` holds: [`claim_timeline()`](../linux_drm_syncobj.cpp) checks `no_acquire_point` then `no_release_point` even with no buffer (unlike [`linux-drm-syncobj-v1`'s `no_buffer`](../../../../wayland-protocols/linux-drm-syncobj-v1.xml)). Only with both points set does `WlSurface::commit()` raise `no_buffer` (missing or null buffer) and then `unsupported_buffer` (`ExtForeignBufferV1` only; `ShmBuffer` is accepted, and `no_surface`/`conflicting_points` are unchecked). A buffer committed with neither point set bypasses timeline validation.

## Shells and window roles

- [`WindowWlSurfaceRole`](../window_wl_surface_role.cpp) creates its `scene::Surface` lazily on first commit (or eagerly on `set_maximized`/`set_fullscreen`/`set_minimized`), latches `surface_ready` once `buffer_size()` is non-empty, and keeps explicit window width and height sticky once set. Destroying `wl_surface` before its role calls `destroy_role()` (destroying the role resource, or only logging a warning in [`LayerSurfaceV1`](../layer_shell_v1.cpp) for Squeekboard) rather than raising `defunct_role_object`.
- [`LayerSurfaceV1`](../layer_shell_v1.cpp) folds anchored margins into window padding plus surface offset, propagating that offset to child [`XdgPopupStable`](../xdg_shell_stable.cpp) `aux_rect`s.
- [`SessionLockV1`](../session_lock_v1.cpp) acquires `scene::SessionLock::try_lock()` and sends `locked` synchronously in its constructor; client disconnect clears `active_lock` without unlocking `scene::SessionLock`, and `Instance::destroy()` inverts `is_active_lock` when checking `invalid_destroy`.
- [`ForeignToplevelManagerV1`](../foreign_toplevel_manager_v1.cpp) and [`ExtForeignToplevelListV1`](../foreign_toplevel_list_v1.cpp) expose only `normal`/`utility`/`freestyle` surfaces on the `application` depth layer with focus enabled (excluding `dialog` and ignoring mapped state); `wlr` runs `ForeignSceneObserver` resource mutations on the scene caller thread, whereas `ext` marshals onto `wayland_executor`. [`DesktopFileManager`](../desktop_file_manager.cpp) resolves Snap (`aa_getpeercon`/`/proc/<pid>/attr/current`) and Flatpak (`.flatpak-info`) desktop IDs before falling back to client `app_id`.
- [`IdleInhibitManagerV1`](../idle_inhibit_v1.cpp) holds an `IdleHub::WakeLock` while the surface is `exposed && !hidden` (note: `Inhibitor::scene_surface` is never assigned, so `visibility_observer` stays registered until `scene::Surface` destruction).

## Input shape, pointer, and seat

- An explicitly empty input region pushes a zero-size rectangle into the accumulator, because Mir treats an empty vector as the default input shape (surface bounds).
- [`WlPointer`](../wl_pointer.cpp) preserves `current_buttons` across `leave` and `enter`, pinning subsurface hit-testing in `enter_or_motion()` while any button is pressed (`leave()` still runs on `mir_pointer_action_leave`, DnD takeover, or surface destruction, and never clears `enter_serial`); on `enter`, button state events precede `wl_pointer.enter`.
- `set_cursor` requires `serial == enter_serial` and allows re-assigning the cursor role (raising `Error::role` on non-cursor roles); re-setting the same cursor surface applies a new `cursor_hotspot` only on the next cursor `wl_surface.commit`. `BufferCursorImage` copies the mapped SHM buffer immediately with `memcpy`.
- `axis_value120` suppresses `axis_discrete` only when `value120 != 0` on v8+ (v8+ still sends deprecated `discrete` when `value120 == 0 && discrete != 0`). If either is sent, a non-value120 `axis` event is also sent (even with value 0) to carry the timestamp; `axis_source` follows them only when supported, non-`none`, and some axis event was sent.
- [`WlSeat`](../wl_seat.cpp) hardcodes `pointer | keyboard | touch` capabilities (TODO). `ListenerList` is keyed by `void*` client and held by `shared_ptr` so lists outlive `WlSeat` when client device resources are destroyed later (`for_each` indexes the vector directly without copying). [`WaylandInputDispatcher`](../wayland_input_dispatcher.cpp) routes only pointer and touch per scene surface; keyboard events go through `WlSeat::KeyboardObserver` and [`KeyboardHelper`](../keyboard_helper.cpp) (which ignores `mir_keyboard_action_repeat` and suppresses unchanged post-`enter` `modifiers`).
- `ConsumedKeyTracker` keys by scancode across devices: once an input trigger consumes a key-down, matching repeats and key-ups are consumed, whereas an unconsumed modifier key-up still forwards to clients even if `handled` is set on release.

## Clipboard, drag-and-drop, and text input

- [`WlDataDevice`](../wl_data_device.cpp) and [`PrimarySelectionDeviceV1`](../primary_selection_v1.cpp) gate selection offers on `WlSeat::FocusListener` keyboard focus, whereas [`DataControlDeviceV1`](../data_control_v1.cpp) observes both clipboards unfocused (suppressing self-echo via `our_source`/`our_primary_source` and registering observers without `wayland_executor`). `WlDataDevice::start_drag()` redirects `PointerEventDispatcher` away from `WlPointer` for the drag lifetime and creates an overlay `DragIconSurface`.
- [`TextInputV1`](../text_input_v1.cpp), [`TextInputV2`](../text_input_v2.cpp), [`TextInputV3`](../text_input_v3.cpp), [`InputMethodV1`](../input_method_v1.cpp), and [`InputMethodV2`](../input_method_v2.cpp) bridge through `scene::TextInputHub`, each capping `commit`/`done` serial-to-`TextInputStateSerial` history at 10 entries. `TextInputV3` synthesizes key events for `InputMethodV1` keysyms via a per-instance `VirtualInputDevice`, `InputPanelSurfaceV1` toggles `mir_window_state_hidden`/`attached` and updates `exclusive_rect` from `get_input_region()[0]`, and `InputMethodGrabKeyboardV2` prepends an `EventFilter` onto `CompositeEventFilter` that consumes all keyboard and resync events.

## Screencopy, capture, and foreign buffers

- [`WlrScreencopyManagerV1`](../wlr_screencopy_v1.cpp) matches damage `Area`s by `FrameParams::operator==` (deliberately excluding `overlay_cursor`), creates the scene change notifier lazily on first `copy_with_damage`, clears `areas` once `areas.size() > 100`, never sends `Flags::y_invert` (the renderer inverts during `set_output_transform`), requires exact-size/stride SHM `argb8888`, and raises `already_used` on a second `copy` or `copy_with_damage`.
- [`ExtImageCopyCaptureSessionV1`](../ext_image_capture_v1.cpp) supports output, `ext_foreign_toplevel_handle_v1`, and pointer-cursor sources, enforces a single active frame (`duplicate_frame`), and requires SHM `argb8888` matching `buffer_size` (reporting `failed(buffer_constraints)` rather than a protocol error). [`ExtForeignBufferSessionV1`](../ext_foreign_buffer_v1.cpp) uses zero-copy `acquire_content()` for an unclipped single-renderable source before falling back to a 4-buffer `argb8888` software-compositing pool.

## Decorations and dialogs

- [`XdgDecorationUnstableV1`](../xdg_decoration_unstable_v1.cpp) shares `ToplevelsWithDecorations` across manager bindings so rebinding cannot bypass `already_constructed` or lose a retained mode. Entries outlive decoration objects unconditionally across versions (only `unconfigured_buffer` attach/has_buffer checks gate on `< v2`), committing while `!live` clears the retained mode, initial `configure` is sent only on `set_mode`/`unset_mode`, and orphaned destruction only logs (issue #3452).
- [`ServerDecorationManager`](../server_decoration_manager.cpp) tracks duplicates per manager binding, sends initial `mode` from `DecorationStrategy` on construction, and echoes client `request_mode` without re-consulting `DecorationStrategy`.
- [`XdgDialogV1`](../xdg_dialog_v1.cpp) tracks `ToplevelsWithDialogs` per manager binding; `set_modal` records `pending_type` and `modal_applied_by_us`, and `unset_modal` restores the prior type only while `modal_applied_by_us` holds and `pending_type` is still `dialog`.

## Activation

- [`XdgActivationV1`](../xdg_activation_v1.cpp) calls `TokenAuthority::revoke_all_tokens()` on key-down and `unfocused()` (alongside `TokenAuthority`'s 3 s `MainLoop` expiry), whereas `focused(session)` revokes only `XdgActivationV1::pending_tokens` not from the newly focused session (leaving externally issued tokens valid). Because `WlSeat::KeyboardObserver` registers before `XdgActivationV1::KeyboardObserver`, a keyboard [`InputTriggerRegistry`](../input_trigger_registry.cpp) key-down issues an `ActionGroup` token that `XdgActivationV1` immediately revokes on the same key-down.
- Non-focused sessions receive a bogus token (`SessionListener::focused_session` is unguarded across threads, while `pending_tokens` is mutex-guarded). `activate()` revokes synchronously and enqueues `shell->raise_surface()` on `MainLoop` (token data may be null for externally issued tokens, and the queued lambda captures raw `surface`/`client`/`seat` pointers).

## Viewporter and output

- [`WpViewporter`](../wp_viewporter.cpp) mutates `Viewport` state immediately rather than double-buffering in `WlSurfaceState`. `set_source(-1,-1,-1,-1)` unsets only cropping and `set_destination(-1,-1)` unsets only scaling; `associate_viewport` rejects surfaces that already have a current or pending viewport. `resolve_viewport()` multiplies by `scale` without applying buffer transform, so cropping, `out_of_buffer` checks, and inferred logical size can be wrong for non-identity transforms.
- [`XdgOutputManagerV1`](../xdg_output_v1.cpp) snapshots `geometry_scale` per client at creation and computes logical position and size as `extents * scale`. It sends `name` once on v2+ (never `description`), sends `xdg_output.done` on `< v3`, and sends `wl_output.done` on v3+ only when the bound `wl_output` is v2+. [`OutputGlobal::bind()`](../output_manager.cpp) invokes `output_config_changed` across all existing listeners on that output while sending `wl_output.done` only on the newly bound instance.
