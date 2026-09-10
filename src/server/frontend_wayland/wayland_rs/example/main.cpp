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
#include "client.h"
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
/// A minimal concrete `Client` for the example. It wraps the raw Rust
/// `WaylandClient` so the `WaylandClientRegistry` can resolve/remove it by id.
///
/// A real frontend would back these methods with an actual Mir session and
/// serial bookkeeping; here they are stubbed since the example only exercises
/// registration and removal.
class ExampleClient : public mir::wayland::Client
{
public:
    explicit ExampleClient(mir::wayland::RawWlClient client) :
        client{std::move(client)}
    {
    }

    auto raw_client() const -> mir::wayland::RawWlClient const& override
    {
        return client;
    }

    auto is_being_destroyed() const -> bool override
    {
        return false;
    }

    auto client_session() const -> std::shared_ptr<mir::scene::Session> override
    {
        return nullptr;
    }

    auto next_serial(std::shared_ptr<MirEvent const>) -> uint32_t override
    {
        return 0;
    }

    auto event_for(uint32_t) -> std::optional<std::shared_ptr<MirEvent const>> override
    {
        return std::nullopt;
    }

    void set_output_geometry_scale(float scale) override
    {
        geometry_scale = scale;
    }

    auto output_geometry_scale() -> float override
    {
        return geometry_scale;
    }

private:
    mir::wayland::RawWlClient client;
    float geometry_scale{1.0f};
};

class WaylandServerNotificationHandler : public mir::wayland::WaylandServerNotificationHandler
{
public:
    explicit WaylandServerNotificationHandler(mir::wayland::WaylandClientRegistry& registry) :
        registry{registry}
    {
    }

    auto client_added(rust::Box<mir::wayland::WaylandClient> wayland_client) -> void override
    {
        try
        {
            std::cout << "Client connected " << wayland_client->pid() << "." << std::endl;
        }
        catch (rust::Error const& error)
        {
            std::cerr << "Failed to get client pid: " << error.what() << std::endl;
        }

        registry.add_client(std::make_shared<ExampleClient>(std::move(wayland_client)));
    }

    auto client_removed(rust::Box<mir::wayland::WaylandClientId> client_id) -> void override
    {
        std::cout << "Client disconnected." << std::endl;
        registry.remove_client(client_id);
    }

private:
    mir::wayland::WaylandClientRegistry& registry;
};

/// Prints when the descriptor it watches becomes readable. The registration is
/// one-shot, so this fires at most once; it reads the pending byte purely to
/// consume it.
class ExampleFdReadyCallback : public mir::wayland::FdReadyCallback
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

class WaylandWorkCallback : public mir::wayland::WorkCallback
{
public:
    explicit WaylandWorkCallback(std::shared_ptr<mir::wayland::WaylandExecutor> const& executor)
        : executor(executor)
    {
    }

    void execute() override
    {
        if (auto const locked = executor.lock())
            locked->execute();
    }

private:
    std::weak_ptr<mir::wayland::WaylandExecutor> executor;
};
}

void run_server(
    rust::Box<mir::wayland::WaylandServer>& server,
    std::shared_ptr<mir::wayland::WaylandExecutor> const& executor)
{
    try
    {
        // The registry owns the Client objects. The notification handler populates it as
        // clients connect/disconnect, and a real GlobalFactory would resolve a client's
        // raw box to its shared Client (via registry.from(...)) when creating objects.
        // `run` blocks until the server stops, so this local outlives the handler the
        // server owns.
        mir::wayland::WaylandClientRegistry registry;

        // TODO: Add a real GlobalFactory here, but we will keep it nullptr for the time being
        //  as that task is involved.
        server->run(
            wayland_socket,
            nullptr,
            std::make_unique<WaylandServerNotificationHandler>(registry),
            std::make_unique<WaylandWorkCallback>(executor));
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

    auto server = mir::wayland::create_wayland_server();

    // The executor lets us schedule arbitrary work onto the server's event loop.
    // Ownership is handed to the server (via run), but it remains valid for the
    // server's lifetime, so we keep a non-owning reference to spawn work with.
    auto executor = std::make_shared<mir::wayland::WaylandExecutor>(*server);

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
        [&server, executor]() mutable
        {
            run_server(server, executor);
        });

    run_client();

    // Queue several functions from this (non event-loop) thread. They are run,
    // in order, on the event-loop thread.
    executor->spawn([] { std::cout << "    Scheduled work 1 ran on the event loop." << std::endl; });
    executor->spawn([] { std::cout << "    Scheduled work 2 ran on the event loop." << std::endl; });
    executor->spawn([] { std::cout << "    Scheduled work 3 ran on the event loop." << std::endl; });

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
