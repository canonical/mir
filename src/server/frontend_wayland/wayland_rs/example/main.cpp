/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "wayland_rs/src/ffi.rs.h"
#include "wayland_executor.h"
#include "wayland_client_registry.h"
#include <mir/executor.h>
#include <wayland-client.h>
#include <array>
#include <chrono>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>
#include <rust/cxx.h>

auto const wayland_socket = "wayland-98";

namespace
{
class WaylandServerNotificationHandler : public mir::wayland_rs::WaylandServerNotificationHandler
{
public:
    explicit WaylandServerNotificationHandler(mir::wayland_rs::WaylandClientRegistry& registry) :
        registry{registry}
    {
    }

    auto client_added(rust::Box<mir::wayland_rs::WaylandClient> wayland_client) -> void override
    {
        try
        {
            std::cout << "Client connected " << wayland_client->pid() << "." << std::endl;
        }
        catch (rust::Error const& error)
        {
            std::cerr << "Failed to get client pid: " << error.what() << std::endl;
        }

        // TODO: build a concrete mir::wayland_rs::Client wrapping `wayland_client`
        //  (e.g. opening a shell session) and register it so it can be resolved when
        //  this client's Wayland objects are created:
        //      registry.add_client(std::make_shared<MyClient>(std::move(wayland_client)));
        //  Creating that concrete Client is deferred work.
        (void)registry;
    }

    auto client_removed(rust::Box<mir::wayland_rs::WaylandClientId> client_id) -> void override
    {
        std::cout << "Client disconnected." << std::endl;
        registry.remove_client(client_id);
    }

private:
    mir::wayland_rs::WaylandClientRegistry& registry;
};

/// Prints when the descriptor it watches becomes readable. The registration is
/// one-shot, so this fires at most once; it reads the pending byte purely to
/// consume it.
class ExampleFdReadyCallback : public mir::wayland_rs::FdReadyCallback
{
public:
    explicit ExampleFdReadyCallback(int fd)
        : fd{fd}
    {
    }

    auto ready() -> void override
    {
        char buffer{};
        while (::read(fd, &buffer, sizeof(buffer)) > 0)
        {
        }
        std::cout << "    Registered fd became readable on the event loop." << std::endl;
    }

private:
    int fd;
};
}

void run_server(
    rust::Box<mir::wayland_rs::WaylandServer>& server,
    std::unique_ptr<mir::wayland_rs::WaylandExecutor> executor)
{
    try
    {
        // The registry owns the Client objects. The notification handler populates it as
        // clients connect/disconnect, and a real GlobalFactory would resolve a client's
        // raw box to its shared Client (via registry.from(...)) when creating objects.
        static mir::wayland_rs::WaylandClientRegistry registry;

        // TODO: Add a real GlobalFactory here, but we will keep it nullptr for the time being
        //  as that task is involved.
        server->run(
            wayland_socket,
            nullptr,
            std::make_unique<WaylandServerNotificationHandler>(registry),
            std::move(executor));
    }
    catch (rust::Error const& error)
    {
        std::cerr << "Failed to run server: " << error.what() << std::endl;
    }
}

void run_client()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto const display = wl_display_connect(wayland_socket);
    if (!display)
    {
        std::cerr << "Failed to connect to Wayland display." << std::endl;
        return;
    }

    wl_display_disconnect(display);
    std::cout << "Client successfully connected." << std::endl;
}

int main()
{
    std::cout << "This is a demo of the C++ bindings to the wayland_rs crate." << std::endl;
    std::cout << "The program is successful if the following happens:" << std::endl;
    std::cout << "    1) The Wayland server gracefully starts on wayland-98." << std::endl;
    std::cout << "    2) The client connects to the server." << std::endl;
    std::cout << "    3) Work scheduled via the executor runs on the event loop." << std::endl;
    std::cout << "    4) A registered file descriptor is reported ready when written to." << std::endl;
    std::cout << "    5) The server stops." << std::endl;
    std::cout << "    6) We exit with exit code 0." << std::endl << std::endl;

    auto server = mir::wayland_rs::create_wayland_server();

    // The executor lets us schedule arbitrary work onto the server's event loop.
    // Ownership is handed to the server (via run), but it remains valid for the
    // server's lifetime, so we keep a non-owning reference to spawn work with.
    auto executor = std::make_unique<mir::wayland_rs::WaylandExecutor>(*server);
    mir::Executor& work_executor = *executor;

    // A pipe whose read end we ask the server to watch. Writing to the write end
    // makes the read end readable, which the server reports via FdReadyCallback.
    std::array<int, 2> fds{};
    if (::pipe2(fds.data(), O_NONBLOCK) != 0)
    {
        std::cerr << "Failed to create pipe." << std::endl;
        return 1;
    }
    auto const read_fd = fds[0];
    auto const write_fd = fds[1];

    server->register_fd_ready_listener(read_fd, std::make_unique<ExampleFdReadyCallback>(read_fd));

    std::thread server_thread(
        [&server, executor = std::move(executor)]() mutable
        {
            run_server(server, std::move(executor));
        });

    run_client();

    // Queue several functions from this (non event-loop) thread. They are run,
    // in order, on the event-loop thread.
    work_executor.spawn([] { std::cout << "    Scheduled work 1 ran on the event loop." << std::endl; });
    work_executor.spawn([] { std::cout << "    Scheduled work 2 ran on the event loop." << std::endl; });
    work_executor.spawn([] { std::cout << "    Scheduled work 3 ran on the event loop." << std::endl; });

    // Make the watched descriptor readable; the server's FdReadyCallback fires
    // on the event-loop thread.
    char const byte = 1;
    if (::write(write_fd, &byte, sizeof(byte)) != sizeof(byte))
        std::cerr << "Failed to write to pipe." << std::endl;

    // Give the event loop a moment to run the scheduled work before stopping.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    server->stop();
    server_thread.join();

    ::close(read_fd);
    ::close(write_fd);

    std::cout << "Success!" << std::endl;

    return 0;
}
