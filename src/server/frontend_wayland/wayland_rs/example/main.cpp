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
#include <wayland-client.h>
#include <iostream>
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
        std::cout << "Client connected " << wayland_client->pid() << "." << std::endl;
    }

    auto client_removed(rust::Box<mir::wayland_rs::WaylandClientId>) -> void override
    {
        std::cout << "Client disconnected." << std::endl;
    }
};
}

void run_server(rust::Box<mir::wayland_rs::WaylandServer>& server)
{
    try
    {
        // TODO: Add a real GlobalFactory here, but we will keep it nullptr for the time being
        //  as that task is involved.
        server->run(wayland_socket, nullptr, std::make_unique<WaylandServerNotificationHandler>());
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
    std::cout << "    3) The server stops." << std::endl;
    std::cout << "    4) We exit with exit code 0." << std::endl << std::endl;

    auto server = mir::wayland_rs::create_wayland_server();
    std::thread server_thread([&] { run_server(server); });

    run_client();

    server->stop();
    server_thread.join();

    std::cout << "Success!" << std::endl;

    return 0;
}
