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
#include <mir/executor.h>
#include <wayland-client.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <rust/cxx.h>

auto const wayland_socket = "wayland-98";

namespace
{
class WaylandServerNotificationHandler : public mir::wayland_rs::WaylandServerNotificationHandler
{
public:
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
    }

    auto client_removed(rust::Box<mir::wayland_rs::WaylandClientId>) -> void override
    {
        std::cout << "Client disconnected." << std::endl;
    }
};
}

void run_server(
    rust::Box<mir::wayland_rs::WaylandServer>& server,
    std::unique_ptr<mir::wayland_rs::WaylandExecutor> executor)
{
    try
    {
        // TODO: Add a real GlobalFactory here, but we will keep it nullptr for the time being
        //  as that task is involved.
        server->run(
            wayland_socket,
            nullptr,
            std::make_unique<WaylandServerNotificationHandler>(),
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
    std::cout << "    4) The server stops." << std::endl;
    std::cout << "    5) We exit with exit code 0." << std::endl << std::endl;

    auto server = mir::wayland_rs::create_wayland_server();

    // The executor lets us schedule arbitrary work onto the server's event loop.
    // Ownership is handed to the server (via run), but it remains valid for the
    // server's lifetime, so we keep a non-owning reference to spawn work with.
    auto executor = std::make_unique<mir::wayland_rs::WaylandExecutor>(*server);
    mir::Executor& work_executor = *executor;

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

    // Give the event loop a moment to run the scheduled work before stopping.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    server->stop();
    server_thread.join();

    std::cout << "Success!" << std::endl;

    return 0;
}
