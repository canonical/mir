# Preface

This file covers the focus, popup-grab, and pointer-confinement behavior implemented by [`AbstractShell`](../abstract_shell.cpp), the base class every shell builds on.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Focus state is guarded by `focus_mutex`

Focus-operation paths access `focus_session`, `focus_surface`, `last_requested_focus_surface`, `notified_keyboard_focus_surface`, `popup_parent`, and `grabbing_popups` under `focus_mutex`. Helpers receive the already-held lock as an unused `unique_lock` parameter rather than re-locking. The destructor is the exception: it reads `notified_keyboard_focus_surface` unlocked.

## §2 - Streams are applied last

`modify_surface` hands `streams` to `Session::configure_streams` only after `WindowManager::modify_surface`, because stream application notifies scene observers that need settled geometry and input region.


# Concepts

## Focus requests versus notifications

- [`set_focus_to()`](../abstract_shell.cpp) is gated on `last_requested_focus_surface`: only a change there re-runs `notify_active_surfaces()` and `set_keyboard_focus_surface()`. `update_focus_locked()` runs on every call, so session focus and `report->input_focus_set_to()` are always refreshed.
- `set_keyboard_focus_surface()` has a second, independent gate on `notified_keyboard_focus_surface`. A request that changes the surface but resolves to the same non-popup keyboard focus still renotifies active surfaces. However, it skips the seat, input-targeter, and confinement work.
- [`focus_next_session()` and `focus_prev_session()`](../abstract_shell.cpp) call `update_focus_locked()` directly, bypassing `set_focus_to()`: they move session focus and the report but leave keyboard focus and active-surface states untouched.

## Keyboard focus and popups

- Active surfaces are the full ancestry of the requested surface (`get_ancestry`), but keyboard focus is `get_non_popup_parent()`: grabbing popups (menu/gloss/tip) are skipped. The HACK comment states this deliberately disobeys xdg-shell to match Weston and avoid breaking Qt submenus, citing issue #2324.
- `set_popup_grab_tree()` sets the popup parent to the toplevel of the given surface. Changing the parent requests close and hides every grabbing popup, but leaves the `grabbing_popups` vector populated.
- `surface_ready()` registers a `mir_window_type_menu` surface as a grabbing popup. `add_grabbing_popup()` closes and hides it immediately when its toplevel is not the current popup parent.

## Pointer confinement

- Confinement is applied to the seat, not the surface. `update_confinement_for()` maps confined states to `input_bounds()`, and locked states to a 1×1 rectangle at the center of `input_bounds()` (the lock emulation).
- On a keyboard-focus change, `set_keyboard_focus_surface()` resets the seat confinement regions. It demotes the previous surface from `*_oneshot` to `mir_pointer_unconfined`, while persistent states remain on the surface.
- `SurfaceConfinementUpdater` is registered only on the keyboard-focus surface. It reapplies `input_bounds()` from `content_resized_to` and `moved_to` only for confined states. Locked surfaces keep the region computed at focus time.
- `modify_surface()` always stores the new `confine_pointer` state and reapplies confinement when the surface is the focused one. A transition between two non-unconfined states proceeds without the `already_constrained` report (TODO).

## Surface modification

- `adjust_size_constraints_for_ssd()` adds frame padding to `width`, `height`, `min_*`, and `max_*`. Only the max values are clamped against `int` overflow (`< int_max - padding`), and min values are not.
- It is called unconditionally from `modify_surface()` with the surface current window and content sizes (rewritten when SSD is toggled). Padding is zero exactly when those sizes are equal.
- `create_surface()` passes the WM a build lambda that records `server_side_decorated` in a `shared_ptr<bool>`. The flag is shared because the WM can invoke the lambda at a time the caller does not control.

## Session teardown

- `close_session()` has no surface list from the session, so it walks `default_surface()`/`surface_after()` into a `std::set` and stops on the first repeat to break cycles. Both the collection and the removal loops are marked as kludges, with a TODO to move that knowledge into `WindowManager`.
