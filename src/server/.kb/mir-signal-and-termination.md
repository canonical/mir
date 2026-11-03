# Preface

This file describes how Mir handles process termination: fatal-signal cleanup in [`run_mir()`](../run_mir.cpp), exception propagation from worker threads, and the main-loop signal self-pipe.

Read the top-level `.kb/agents.md` file before continuing below.


# Laws

## §1 - Fatal cleanup is terminal

The [`fatal_signal_cleanup`](../run_mir.cpp) handler performs emergency cleanup and must not return. It restores and invokes the previous handler, re-raises the signal, or aborts through `fatal_error()`. Any handler that tries to continue after a fatal signal is reported as a fatal error.

## §2 - Handler install is refcounted

`run_mir()` installs the fatal-signal handlers only at the outermost nesting level and restores the previous handlers only on the last one. Nested `run_mir()` calls must not double-install.

## §3 - First termination exception wins

`terminate_with_current_exception()` records only the first exception and ignores later ones. `check_for_termination_exception()` rethrows it on the main thread.


# Concepts

## Fatal signal handling

[`run_mir()`](../run_mir.cpp) intercepts SIGQUIT, SIGABRT, SIGFPE, SIGSEGV and SIGBUS with `fatal_signal_cleanup` (SA_SIGINFO). The handler writes a fixed warning and a JSON crash event to stderr, runs emergency cleanup, and unblocks the handled signal. It restores the saved handler, then either calls an SA_SIGINFO predecessor directly (to preserve its `siginfo`) or re-raises. Saved handlers live in the atomic array inside `SignalHandlers`.

If the parent already ignores SIGHUP (for example, under `nohup`), `run_mir()` inherits that ignore and registers only SIGINT and SIGTERM with the main-loop terminator. Otherwise it registers all three of SIGHUP, SIGINT, and SIGTERM, so any of them terminates.

## Termination exceptions

[`terminate_with_current_exception()`](../terminate_with_current_exception.cpp) is the thread-pool unhandled-exception handler: it stores the first exception under a mutex, logs it at critical severity, and sends itself SIGTERM. `clear_termination_exception()` resets the slot at the start of each `run_mir()`. See [`mir-server.md`](mir-server.md) for the shutdown path that rethrows it through `check_for_termination_exception()`.

## Main-loop signal sources

[`SignalSources`](../../include/server/mir/glib_main_loop_sources.h) uses a self-pipe. The signal handler writes a `SignalPid{sig, pid}` to a non-blocking write fd, and the main loop dispatches a read fd. Up to ten write fds live in a static atomic array, one per live `SourceRegistration`, and exceeding this limit throws "too many main loops".

The handler scans the atomic slots with a documented benign race. A slot can change between the `>= 0` check and the `write()`. The documented worst case is a harmless `write()` to fd `-1`.

## Emergency cleanup ordering

[`DefaultEmergencyCleanup::operator()`](../default_emergency_cleanup.cpp) snapshots `num_handlers` before iterating, so handlers registered during cleanup do not run in that pass. The emergency cleanup is held weakly by `run_mir()` and consumed on first use, so it runs at most once.

## Wayland extension policy

[`DefaultServerConfiguration`](../default_server_configuration.cpp) builds a filter map that denies supported non-standard extensions and allows standard ones. The filter also allows clients whose `process_id()` equals `getpid()`. Extensions in neither list fall through to allow.
