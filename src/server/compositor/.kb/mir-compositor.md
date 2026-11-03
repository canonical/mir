# Preface

This file distills the invariants of Mir's compositor: buffer arbitration, the compositing
loop, direct-presentation bypass, occlusion, screen shooting, and configuration wiring.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Compositing thread entry point

[`CompositingFunctor::operator()`](../multi_threaded_compositor.cpp) must remain `noexcept`
(LP: #1237332). Exceptions before `started.set_value()` are captured into the `started` promise.
Exceptions after it call `mir::terminate_with_current_exception()`.


# Concepts

## Multi-monitor buffer arbitration

[`MultiMonitorArbiter`](../multi_monitor_arbiter.h) holds a current and a next submission plus
per-compositor "seen current" slots (`current_buffer_users`). `compositor_acquire()` advances
current to next only when there is no current submission or the caller already saw current. Otherwise
it re-serves current. A compositor marks its seen-current slot lazily on first buffer access
through `claim_buffer()`, not at acquire. Advancing clears the seen-current slots, so the queue
advances at the rate of the fastest compositor. Recreated compositors have not seen current, so they
re-serve it and can stall until something else changes (`wp_presentation` and `wp_fifo` expect
single-output synchronization).

## Compositing threads

[`MultiThreadedCompositor`](../multi_threaded_compositor.cpp) runs one `CompositingFunctor` per
`DisplaySyncGroup`, spawned on `thread_pool_executor`. State transitions use CAS:
stopped to starting to started, and started to stopping to stopped. `on_unwind` restores the prior
state if construction or teardown throws. The scene observer is added after thread creation and
removed before destruction. `stop()` sets `compose_on_start = true` so a restart unblocks waiting clients.

`CompositingFunctor` primes every renderable buffer at start-up so each compositor is marked
seen-current before the loop. `running` is atomic. A value of `force_sleep < 0` uses
`group.recommended_sleep()`. Both `wait_until_started()` and `wait_until_stopped()` time out after 10 seconds.

## Direct presentation bypass

[`DefaultDisplayBufferCompositor::composite()`](../default_display_buffer_compositor.cpp) attempts
`display_sink.overlay()` only when the output filter is `mir_output_filter_none` and the transform is
identity. `import_display_elements()` stops at the first renderable whose buffer cannot become a
framebuffer, so a short list forces the GL render path. Buffers stay alive through `visible_elements` and
`renderable_list`. Calling `renderable_list.clear()` before `post()` provides an early-release optimization
(LP: #1395421, LP: #1561418).

## Occlusion

[`split_occluded_and_visible()`](../occlusion.cpp) walks elements topmost-first (reverse order). A
renderable with a non-identity transform is never occluded. One whose clipped window is empty is
always occluded. Coverage grows only from fully opaque (`alpha == 1.0f`) renderables. Shaped ones
contribute their `opaque_region` (empty if absent), and unshaped ones contribute their clipped window.
All regions are clipped to the view area.

## Screen shooting

[`BasicScreenShooter`](../basic_screen_shooter.cpp) reuses one `Renderer`, rebuilding it only on a
pixel-format change. `render()` is mutex-serialized, and `capture()` always invokes its callback
(with `nullopt` on failure) through `executor.spawn` holding a `weak_ptr`. If a buffer handed to
`OneShotBufferDisplayProvider` is not consumed by `alloc_fb()` or cleared during capture, every later
capture fails. Provider selection requires both `suitability_for_display` and
`suitability_for_allocator` to be supported.

## Configuration

`MIR_PIN_COMPOSITING_TO` pins compositing to a single GL provider and warns if that platform lacks GL.
The screen-shooter factory falls back to [`NullScreenShooterFactory`](../null_screen_shooter_factory.cpp)
when no platform offers GL.
