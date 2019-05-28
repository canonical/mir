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

using mir::examples::ServerDecorationCreateCallback;

namespace mw = mir::wayland;

namespace
{
struct ServerDecoration :
    mw::ServerDecoration
{
    ServerDecoration(wl_resource* new_resource) :
        mw::ServerDecoration::ServerDecoration(new_resource, Version<1>())
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

struct ServerDecorationManager : mw::ServerDecorationManager::Global
{
    static int const interface_supported = 1;

    ServerDecorationManager(struct wl_display* display) :
        Global(display, Version<interface_supported>())
    {
    };

    ServerDecorationManager(struct wl_display* display, ServerDecorationCreateCallback callback) :
        Global(display, Version<interface_supported>()),
        callback{callback}
    {
    };

    class Instance : public mw::ServerDecorationManager
    {
    public:
        Instance(wl_resource* new_resource, ::ServerDecorationManager* manager)
            : mw::ServerDecorationManager(new_resource, Version<1>()),
              manager{manager}
        {
        }

    private:
        void create(wl_resource* new_resource, wl_resource* surface) override
        {
            new ServerDecoration{new_resource};
            manager->callback(client, surface);
        }

        ::ServerDecorationManager* const manager;
    };

    void bind(wl_resource* new_resource)
    {
        new Instance{new_resource, this};
    }

    ServerDecorationCreateCallback const callback = [](auto, auto) {};
};

int const ServerDecorationManager::interface_supported;
}

auto mir::examples::server_decoration_extension() -> miral::WaylandExtensions::Builder
{
    return
        {
            wayland::ServerDecorationManager::interface_name,
            [](miral::WaylandExtensions::Context const* context)
                {
                    return std::make_shared<ServerDecorationManager>(context->display());
                }
        };
}

auto mir::examples::server_decoration_extension(ServerDecorationCreateCallback callback) -> miral::WaylandExtensions::Builder
{
    return
        {
            wayland::ServerDecorationManager::interface_name,
            [callback](miral::WaylandExtensions::Context const* context)
                {
                    return std::make_shared<ServerDecorationManager>(context->display(), callback);
                }
        };
}
