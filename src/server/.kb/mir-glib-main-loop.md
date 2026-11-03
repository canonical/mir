# Preface

This file documents the GLib-backed `MainLoop` implementation: its source wrappers, dispatch guarantees, and the alarm state machine layered on top.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Source teardown

`g_source_destroy()` does not guarantee that an in-flight dispatch has finished, so any source whose callback must not run after teardown must call `GSourceHandle::ensure_no_further_dispatch()` first.

## §2 - Timer lock order

`TimerGSource::dispatch` must acquire the handler's lock before `ctx.mutex` to preserve the caller's lock order.


# Concepts

## Source handles

[`GSourceHandle`](../../include/server/mir/glib_main_loop_sources.h) wraps a `GSource*` plus a `terminate_dispatch` callback. Its destructor calls only `g_source_destroy()` and `g_source_unref()`. [`ensure_no_further_dispatch()`](../glib_main_loop_sources.cpp) invokes `terminate_dispatch` for the stronger guarantee.

[`FdSources`](../glib_main_loop_sources.cpp) and [`add_timer_gsource`](../glib_main_loop_sources.cpp) register a `terminate_dispatch` callback that sets an `enabled` flag under a `std::recursive_mutex`. A racing dispatch observes `enabled == false` and skips the handler. That callback runs only through `ensure_no_further_dispatch()`. `GSourceHandle` destruction and assignment do not invoke it. Therefore, the guarantee applies only where callers request it explicitly (for example, `FdSource` destruction, and alarm destruction or cancellation).

## Server action sources

[`add_server_action_gsource`](../glib_main_loop_sources.cpp) dispatches only when `should_dispatch(owner)` is true. This mechanism allows `pause_processing_for` and `resume_processing_for` to gate enqueued actions per owner.

If finalization occurs before dispatch, `ServerActionGSource::finalize` leaks `ctx`. The action can reference code in an unloaded DSO, and destroying it will crash.

## Main loop lifecycle

[`GLibMainLoop::run()`](../glib_main_loop.cpp) loops on `g_main_context_iteration` and calls `before_iteration_hook` each pass. `stop()` installs a `G_PRIORITY_HIGH` idle source that clears `running_` and drains `run_on_halt_queue`.

If `running_` is false, `enqueue_with_guaranteed_execution` runs the action inline. Otherwise it queues the action and adds a `G_PRIORITY_DEFAULT` idle source. That source pops and runs the oldest queued action during normal dispatch. The `stop()` drain runs only actions still queued when the loop stops. `running_` is `std::atomic<bool>`. The stop drain runs unlocked because other threads observe `running_ == false` before touching the queue.

`reprocess_all_sources()` relies on GLib priority ordering. It sets `before_iteration_hook` from a `G_PRIORITY_HIGH` idle source to avoid concurrent access. That hook installs a `G_PRIORITY_LOW` idle source, so every higher-priority source runs first.

`execute_with_context_as_thread_default` busy-waits (documented case 3) when the loop stops. The implementation avoids `g_main_context_wait()` because GLib silently deprecated it.

## Alarms

[`AlarmImpl`](../glib_main_loop.cpp) is a one-shot resettable timer over `add_timer_gsource`. `cancel()` clears the source only when `state_ == pending`. `reschedule_for()` returns whether the previous state was `pending`. `reschedule_in()` computes `clock->now() + delay`.

[`AlarmFactory::create_repeating_alarm`](../time/alarm_factory.cpp) captures a `weak_ptr` to the alarm it creates, so a repeating alarm does not keep itself alive through its own callback.
