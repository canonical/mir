# Preface

This file maps the server startup and shutdown path and its resource lifetimes.

Read the top-level `.kb/agents.md` file before continuing below.


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
