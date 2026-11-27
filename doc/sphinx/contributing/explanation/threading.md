(mir-threading)=

# Threading Model

This is an (incomplete) guide to the threading model of Mir's components.

## Input Platforms

The `DefaultInputManager` is solely responsible for interacting with the loaded `mir::input::Platform`.
All methods of `mir::input::Platform` are called from a single, dedicated thread, and the
`mir::input::Platform::dispatchable()` is only ever dispatched from that thread.

Methods of `mir::input::InputDevice` may be called from arbitrary threads.

## Wayland

All Wayland objects must be accessed on the thread running the Wayland main loop. In particular,
`mir::wayland::Weak<>::value()` must only be called from the Wayland thread, and the values of
`Weak<>::is()` and `Weak<>::operator bool` are not reliable off the Wayland thread.

Most objects in `frontend::wayland` are provided with a `mir::Executor` that executes the
provided functors on the Wayland thread, typically stored as `wayland_executor` field.
This executor is eager — calling `wayland_executor.spawn(functor)` from the thread executing
the Wayland main loop itself will invoke `functor` immediately rather than deferring execution.

## Graphics

In general, methods of objects in the `mir::graphics` namespace can be called from arbitrary
threads.

Most objects and free-functions in the `mir::graphics::gl` namespace expect to be called with
an (E)GL context current on the thread. A notable exception here is that objects *cannot* expect
to be destroyed on a thread with an appropriate context current.

For this purpose, an `EGLContextExecutor` is provided. This is a `mir::Executor` that executes
the functor from `executor.spawn(functor)` in a thread with an EGL context current. Objects that
wrap GL resources must delegate their cleanup to an `EGLContextExecutor` in their destructor to
ensure that the GL calls (such as `glDeleteTextures`) do not silently fail, leaking GL resources.
