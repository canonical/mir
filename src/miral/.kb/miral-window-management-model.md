# Preface

This file maps the MirAL window-management model: how the controller, windows, and window info relate, and the invariants of their public surfaces.
It covers the locking contract, `Window` identity/lifetime, `WindowInfo` surface mutation and resize constraint order, plus the touch emulator and window-management trace.

IMPORTANT: Read the `.kb/agents.md` file.


# Laws

## §1 - Model access locking

- A thread must hold the `BasicWindowManager` mutex to call "Update Model" methods of [`WindowManagerToolsImplementation`](../window_manager_tools_implementation.h). Call them from a `WindowManagementPolicy` callback or inside an `invoke_under_lock()` callback.
- Never call `invoke_under_lock()` from a thread that already holds the lock: the mutex is a non-recursive `std::mutex` (held through `std::lock_guard` in [`Locker`](../basic_window_manager.cpp)), so it deadlocks.


# Concepts

## Window identity and lifetime

- [`Window`](../window.cpp) wraps a shared `Self`. Identity is `Self`-pointer identity, not surface identity.
- `operator==(Window, Window)` compares the shared `Self`. Copies compare equal, but independently constructed wrappers around the same surface are unequal. `operator==(shared_ptr<Surface>, Window)` compares the locked surface.
- `operator<` uses `owner_before` on the `Self`.
- `Self` holds only `weak_ptr`s to the session and surface. `operator shared_ptr<Surface>()` locks the weak reference and can yield null.

## WindowInfo construction and surface mutation

- [`WindowInfo(Window, WindowSpecification)`](../window_info_internal.cpp) mutates the surface on construction: it sets min/max width/height from the parameters (defaults when absent).
- The min/max width/height setters clamp to `[default_min, default_max]`. Aspect-ratio setters clamp each dimension to fit in a `long` to avoid overflow in `constrain_resize`.

## constrain_resize order

- [`constrain_resize()`](../window_info.cpp) applies, in order: min/max clamps, width/height increment rounding, min-aspect correction, then max-aspect correction.
- It finally shifts `top_left` for left/top-edge resizes, so the opposite edge stays fixed.

## restore_rect

- `restore_rect()` is lazily computed and cached on first access.
- A restored window keeps its current size. A vertically maximized window keeps its current width and
  takes the midpoint height; a horizontally maximized window keeps its current height and takes the
  midpoint width. A maximized or fullscreen window takes the midpoint in both dimensions.
- The midpoint is between the current size and the surface's minimum size. `top_left` is shifted by
  half the size difference so the restored rectangle is centered within the current one
  ([`window_info.cpp`](../window_info.cpp)).

## Touch emulator

- [`TouchEmulator`](../touch_emulator.cpp): Ctrl-modified pointer events pass through. The emulator consumes all other pointer events.
- The system dispatches pointer-to-touch events through `main_loop->enqueue`, not inline. Calling `sink->handle_input()` inside the composite event filter re-enters the filter and deadlocks.
- The system registers the virtual touch device lazily (in `init` when already active, or in `activate`). The `touch_id` value is always `0`.

## Window-management trace

- [`WindowManagementTrace`](../window_management_trace.cpp) input handlers (keyboard/touch/pointer) install a `log_input` lambda that logs once on the next call, then clears itself.
- `advise_begin()` clears `log_input` and resets `trace_count`. `advise_end()` logs `====` when `trace_count > 0`.
