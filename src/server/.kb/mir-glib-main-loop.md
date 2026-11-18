# Preface

This file documents the GLib-backed `MainLoop`: its source wrappers, dispatch guarantees, and alarms.

IMPORTANT: Read the `.kb/agents.md` file.


# Laws

## §1 - Source teardown

`g_source_destroy()` does not guarantee that an in-flight dispatch has finished, so any source whose callback must not run after teardown must call `GSourceHandle::ensure_no_further_dispatch()` first.

## §2 - Timer lock order

`TimerGSource::dispatch` must acquire the handler's lock before `ctx.mutex` to preserve the caller's lock order.


# Concepts

## Source handles

[`GSourceHandle`](../../include/server/mir/glib_main_loop_sources.h) wraps a `GSource*` plus a `terminate_dispatch` callback. Its destructor only calls `g_source_destroy()` and `g_source_unref()`; [`ensure_no_further_dispatch()`](../glib_main_loop_sources.cpp) invokes the callback for the stronger guarantee. [`FdSources`](../glib_main_loop_sources.cpp) and [`add_timer_gsource`](../glib_main_loop_sources.cpp) register a callback that sets an `enabled` flag under a `std::recursive_mutex`, so a racing dispatch skips the handler.

## Server action sources

[`add_server_action_gsource`](../glib_main_loop_sources.cpp) dispatches only when `should_dispatch(owner)` is true, which lets `pause_processing_for` and `resume_processing_for` gate enqueued actions per owner. If finalization occurs before dispatch, `ServerActionGSource::finalize` leaks `ctx`; the action can reference code in an unloaded DSO, and destroying it will crash.

## Main loop lifecycle

[`GLibMainLoop::run()`](../glib_main_loop.cpp) loops on `g_main_context_iteration` and calls `before_iteration_hook` each pass. `stop()` only schedules a `G_PRIORITY_HIGH` idle source; `running_` is cleared and `run_on_halt_queue` drained when that source is dispatched, so returning from `stop()` does not guarantee shutdown has completed. `enqueue_with_guaranteed_execution` runs inline when `running_` is false, otherwise queues a `G_PRIORITY_DEFAULT` idle source. `reprocess_all_sources()` uses priority ordering. `execute_with_context_as_thread_default` busy-waits when the loop stops.

## Alarms

[`AlarmImpl`](../glib_main_loop.cpp) is a one-shot resettable timer over `add_timer_gsource`. `cancel()` clears the source only when `state_ == pending`; `reschedule_for()` returns whether the previous state was `pending`; `reschedule_in()` computes `clock->now() + delay`. [`AlarmFactory::create_repeating_alarm`](../time/alarm_factory.cpp) captures a `weak_ptr` to the alarm it creates, so a repeating alarm does not keep itself alive through its own callback.
