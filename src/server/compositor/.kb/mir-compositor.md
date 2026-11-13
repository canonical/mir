# Preface

This file distills the invariants of Mir's compositor: buffer arbitration, the compositing
loop, direct-presentation bypass, occlusion, screen shooting, and configuration wiring.

IMPORTANT: Read the `.kb/agents.md` file.


# Laws

## §1 - Compositing thread entry point

[`CompositingFunctor::operator()`](../multi_threaded_compositor.cpp) must remain `noexcept`
(LP: #1237332). Exceptions before `started.set_value()` are captured into the `started` promise;
exceptions after it call `mir::terminate_with_current_exception()`.


# Concepts

## Multi-monitor buffer arbitration

[`MultiMonitorArbiter`](../multi_monitor_arbiter.h) holds current and next submissions plus
per-compositor "seen current" slots. `compositor_acquire()` advances to next only when no current
exists or the caller saw current; otherwise it re-serves current. Advancing clears the slots, so the
queue advances at the fastest compositor's rate. Recreated compositors re-serve current and can stall.

## Compositing threads

[`MultiThreadedCompositor`](../multi_threaded_compositor.cpp) runs one `CompositingFunctor` per
`DisplaySyncGroup` on `thread_pool_executor`. State transitions use CAS (stopped→starting→started,
started→stopping→stopped); `on_unwind` restores the prior state on throw. The scene observer is added
after thread creation and removed before destruction. `stop()` sets `compose_on_start = true` so a
restart unblocks waiting clients.

## Direct presentation bypass

[`DefaultDisplayBufferCompositor::composite()`](../default_display_buffer_compositor.cpp) attempts
`display_sink.overlay()` only when the output filter is `mir_output_filter_none` and the transform is
identity. `import_display_elements()` stops at the first renderable whose buffer cannot become a
framebuffer, so a short list forces the renderer path (GL or blitter). Buffers stay alive through
`visible_elements` and `renderable_list`; `renderable_list.clear()` before `post()` is an early-release
optimization (LP: #1395421, LP: #1561418).

## Occlusion

[`split_occluded_and_visible()`](../occlusion.cpp) walks elements topmost-first. A renderable with a
non-identity transform is never occluded; one whose clipped window is empty is always occluded.
Coverage grows only from fully opaque (`alpha == 1.0f`) renderables. Shaped ones contribute their
`opaque_region`; a shaped renderable without one contributes nothing. Unshaped ones contribute their
clipped window. All regions are clipped to the view area.

## Screen shooting

[`BasicScreenShooter`](../basic_screen_shooter.cpp) reuses one `Renderer`, rebuilding it only on a
pixel-format change. `render()` is mutex-serialized, and `capture()` always invokes its callback
(with `nullopt` on failure) through `executor.spawn` holding a `weak_ptr`. A buffer handed to
`OneShotBufferDisplayProvider` that is not consumed by `alloc_fb()` or cleared during capture makes
every later capture fail.

## Configuration

`MIR_PIN_COMPOSITING_TO` pins the rendering platform: it restricts GL and blitter providers and
buffer allocation to that platform. If the platform lacks GL, Mir warns but keeps the original GL
provider list. The screen-shooter factory falls back to
[`NullScreenShooterFactory`](../null_screen_shooter_factory.cpp) when no platform offers GL.
