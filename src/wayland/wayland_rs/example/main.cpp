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
#include <print>
#include <thread>

auto const WAYLAND_SOCKET = "wayland-98";

void run_server()
{
    auto server = mir::wayland_rs::create_wayland_server();
    rust::Str const socket = WAYLAND_SOCKET;

    server->run(socket);
}

int run_client()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto const display = wl_display_connect(WAYLAND_SOCKET);
    if (!display)
    {
        std::print("Failed to connect to Wayland display\n");
        return 1;
    }
    
    wl_display_disconnect(display);
    return 0;
}

int main()
{
    std::thread server_thread(run_server);

    run_client();
    std::print("Running wayland server, use Ctrl + C to exit\n");
    server_thread.join();

    return 0;
}
