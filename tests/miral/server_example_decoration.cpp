/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "server_example_decoration.h"
#include "server-decoration_wrapper.h"

namespace
{
struct ServerDecoration :
    mir::wayland::ServerDecoration
{
    ServerDecoration(struct wl_client *client, struct wl_resource *parent, uint32_t id) :
        mir::wayland::ServerDecoration::ServerDecoration(client, parent, id)
    {
        send_mode_event(decoration_mode);
    }

    void release() override
    {
        destroy_wayland_object();
    }

    void request_mode(uint32_t mode) override
    {
        if (mode != decoration_mode)
            decoration_mode = mode;

        send_mode_event(decoration_mode);
    }

    uint32_t decoration_mode = Mode::Server;
};

struct ServerDecorationManager :
    mir::wayland::ServerDecorationManager
{
    using mir::wayland::ServerDecorationManager::ServerDecorationManager;

    void create(wl_client *client, wl_resource *resource, uint32_t id, wl_resource */*surface*/) override
    {
        new ServerDecoration{client, resource, id};
    }
};
}

auto mir::examples::server_decoration_extension(
    wl_display* display,
    miral::WaylandExtensions::Executor const& run_on_wayland_mainloop)
-> std::shared_ptr<void>
{
    (void)run_on_wayland_mainloop;
    return std::make_shared<ServerDecorationManager>(display, 1);
}
