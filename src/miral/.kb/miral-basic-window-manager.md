# Preface

This file covers the invariants of MirAL's default window manager, [`BasicWindowManager`](../basic_window_manager.cpp). It covers locking, workspace lifetime, MRU, state, zone, and placement quirks.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Locking

[`Locker`](../basic_window_manager.cpp) brackets its locked scope with `policy->advise_begin()`/`advise_end()`. `handle_request_move`/`handle_request_resize` are the only entries that take a bare `std::lock_guard` and skip the advise bracket. The raw `info_for()`/`for_each_application()` accessors take no lock, so callers must already hold it.

## §2 - Workspace destruction

`Workspace::~Workspace` must never lock the BWM mutex because workspaces can die unsynchronized. It queues its own `weak_ptr` onto `DeadWorkspaces` under `dead_workspaces_mutex` instead.


# Concepts

## Dead workspace reaping

[`Locker`](../basic_window_manager.cpp)'s constructor drains `DeadWorkspaces::workspaces` and erases each dead workspace from the `workspaces_to_windows` bimap. This defers bimap cleanup to the next locked entry.

## Active-window enable flag

`active_window()` returns `mru_active_windows.top()` only while `allow_active_window` is true. Otherwise it returns a null `Window`. [`select_active_window()`](../basic_window_manager.cpp) assigns `allow_active_window = hint`, so Lomiri selecting a null window disables the active window.

## MRU maintenance

[`surface_ready()`](../basic_window_manager.cpp) inserts the ready window below the MRU top (`insert_below_top`). It inserts only when a top exists and differs from the window (`if (top && top != window)`). With an empty MRU, it inserts nothing. Windows often never receive selection due to focus-stealing prevention. Relying on selection alone will leave them out of `mru_active_windows` and break alt+tab.

## Hidden/minimized refocus hack

[`set_state()`](../basic_window_manager.cpp) sets `window_info.state(mir_window_state_hidden)` before calling `select_active_window()` to find a new focus target. This prevents the search from restoring the window being minimized. The function applies the real `value` (which can be `mir_window_state_minimized`) afterwards.

## Place before resize

[`place_and_size()`](../basic_window_manager.cpp) moves the tree to the new position before resizing it, so input events synthesized during the resize (for example, pointer enter) carry correct surface-local coordinates.

## Display areas and application zones

`display_areas` generally maps 1:1 with outputs, but outputs sharing a `logical_group_id` merge into one area (see the [header](../basic_window_manager.h) comment). `zone_policy_knows_about` records the last zone the policy was told about.

Output create, update, or delete operations set `application_zones_need_update`. [`advise_output_end()`](../basic_window_manager.cpp) defers zone recomputation and calls `update_application_zones_and_attached_windows()`.

## Attached-window placement order

Per area, `update_application_zones_and_attached_windows()` places windows in order: (1) attached windows that ignore exclusion zones, (2) full-width attached windows, (3) other attached windows, (4) maximized windows. Groups (2) and (3) progressively shrink the shared zone rect through `apply_exclusive_rect_to_application_zone()`. Maximized windows use the final zone.

## Refocus cascade

[`refocus()`](../basic_window_manager.cpp) tries targets in order:
1. The `hint`.
2. MRU windows in a shared workspace.
3. `can_activate_window_for_session()` (MRU within the same application).
4. MRU across all applications.
5. `focus_next_application()`.

## Placement and cursor quirks

`constrain_to()` honors `MIR_ANCHOR_RECTANGLE_UNCONSTRAINED` to let LXQt panel anchors sit outside their window geometry. `move_cursor_to()` synthesizes pointer motion through the virtual `pointer_device`, spawned on `linearising_executor`. `swap_tree_order()` refuses to swap overlapping trees and logs an error instead.
