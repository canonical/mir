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

#include "wayland_rs/src/lib.rs.h"
#include <wayland-client.h>
#include <iostream>
#include <thread>

auto const WAYLAND_SOCKET = "wayland-98";

void run_server(rust::Box<mir::wayland_rs::WaylandServer>& server)
{
    server->run(WAYLAND_SOCKET);
}

void run_client()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto const display = wl_display_connect(WAYLAND_SOCKET);
    if (!display)
    {
        std::cout << "Failed to connect to Wayland display" << std::endl;
        return;
    }

    wl_display_disconnect(display);
}

int main()
{
    std::cout << "This is a demo of the C++ bindings to the wayland_rs crate." << std::endl;
    std::cout << "The program is successful if the following happens:" << std::endl;
    std::cout << "    1) The Wayland server gracefully starts on wayland-98." << std::endl;
    std::cout << "    2) The client connects to the server." << std::endl;
    std::cout << "    3) The server stops." << std::endl;
    std::cout << "    4) We exit with exit code 0." << std::endl;

    auto server = mir::wayland_rs::create_wayland_server();
    std::thread server_thread([&] { run_server(server); });

    run_client();

    server->stop();
    server_thread.join();

    std::cout << "Success!" << std::endl;

    return 0;
}
