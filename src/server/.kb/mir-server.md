# Preface

This file maps the server startup and shutdown path and its resource lifetimes.

IMPORTANT: Read the `.kb/agents.md` file.


# Concepts

## Startup and shutdown

[`Server::apply_settings()` and `Server::run()`](../server.cpp) create the server configuration, run pre-init callbacks,
and call `run_mir()`. [`run_mir()`](../run_mir.cpp) constructs a `DisplayServer`, calls the init callback, and runs it.
On normal return, it quiesces the thread pool and checks for a termination exception.

[`DisplayServer::run()`](../display_server.cpp) starts the compositor, input manager, input dispatcher, Wayland connector,
and Xwayland connector in that order. After the main loop returns, it stops them in reverse order.
`DisplayServer::stop()` calls the stop callback before it stops the main loop.

## Resource lifetimes

[`DisplayServer::Private`](../display_server.cpp) retains the graphics platforms and emergency cleanup while the server exists.
[`Server::run()`](../server.cpp) resets the server configuration after exception handling.
The configuration keeps platform libraries loaded while the server processes an exception from a platform library.

## Compositor performance reporting

`Server::override_the_compositor_report()` installs a report through the normal cached component factory.
The logging compositor report provides a protected `report_statistics()` hook with per-display interval totals,
after an initial baseline interval. The hook runs under the report mutex and must not re-enter the report.
The compositor performance test uses this hook with `AsyncServerRunner`; only renderer/mode metadata is read from logs.
Its opt-in CTest entries exercise both `MIR_SERVER_LOGGING` settings using `xvfb-run` and the X11 platform.
The virtual platform has no display sinks, so it cannot produce compositor frame timings.
