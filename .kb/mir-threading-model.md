# Preface

This file distills Mir's cross-component threading constraints for input platforms, Wayland objects,
and graphics resources. The broader guide is
[`threading.md`](../doc/sphinx/contributing/explanation/threading.md#mir-threading).

IMPORTANT: Read the `.kb/agents.md` file.


# Laws

## §1 - Input platform affinity

Only the dedicated `DefaultInputManager` thread may call methods of `mir::input::Platform` or
dispatch its `dispatchable()`. `mir::input::InputDevice` methods may be called from arbitrary
threads.

## §2 - Wayland object affinity

Access Wayland objects only from the thread running the Wayland mainloop. In particular,
`mir::wayland::Weak<>::value()` must run on that thread; `Weak<>::is()` and its boolean conversion
are not reliable from other threads.

## §3 - GL context and cleanup

Most `mir::graphics::gl` objects and functions require an (E)GL context current on the calling
thread. Do not assume a GL-resource-owning object's destructor runs with a suitable context:
delegate GL cleanup through `EGLContextExecutor`.


# Concepts

## Exclusive access

Prefer `mir::Synchronised<T>` for shared data that requires exclusive access, rather than keeping a
`std::mutex` and its guarded data as separate members. It owns the data it protects and its `lock()`
returns an RAII handle that provides access while holding the lock
([`synchronised.h`](../include/core/mir/synchronised.h)).

`mir::graphics` methods are generally callable from arbitrary threads, but this is not universal:
follow component-specific constraints too. For example, [`WlDmaBufBuffer`](../src/platform/graphics/linux_dmabuf.cpp)
is explicitly limited to the Wayland thread. In `frontend::wayland`, the `wayland_executor` commonly
marshals work to the Wayland thread, but it is eager: `spawn()` called from that thread invokes the
functor immediately instead of deferring it.
