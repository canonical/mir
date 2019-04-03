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

#include "xdg-decoration-unstable-v1.h"
#include "xdg-decoration-unstable-v1_wrapper.h"

#include <miral/window.h>

#include <thread>

namespace
{
struct ToplevelDecorationV1 : mir::wayland::ToplevelDecorationV1
{
    ToplevelDecorationV1(struct wl_client *client, struct wl_resource *parent, uint32_t id, wl_resource* toplevel) :
        mir::wayland::ToplevelDecorationV1::ToplevelDecorationV1(client, parent, id),
        toplevel{toplevel}
    {
        send_configure_event(mode);
        if (auto window = miral::window_for(client, toplevel))
            puts("###################### window not null ######################");
        else
            puts("######################## window NULL ########################");
    }

    void destroy() override
    {
        destroy_wayland_object();
    }

    ~ToplevelDecorationV1() override
    {
        test.join();
    }

    void set_mode(uint32_t mode) override
    {
        this->mode = mode;
        send_configure_event(mode);
    }

    void unset_mode() override
    {
        mode = Mode::server_side;
        send_configure_event(mode);
    }

    uint32_t mode = Mode::server_side;
    wl_resource* const toplevel;

    std::thread test{[this]
        {
            // We put this on a thread and sleep because we need to wait for the client
            // to 'commit' before the Window object exists
            std::this_thread::sleep_for(std::chrono::seconds{1});
            if (auto window = miral::window_for(client, toplevel))
                puts("###################### window not null ######################");
            else
                puts("######################## window NULL ########################");
        }};
};

struct DecorationManagerV1 : mir::wayland::DecorationManagerV1
{
    static int const interface_supported = 1;

    using mir::wayland::DecorationManagerV1::DecorationManagerV1;

    void destroy(struct wl_client* /*client*/, struct wl_resource* resource) override
    {
        destroy_wayland_object(resource);
    }

    void get_toplevel_decoration(wl_client* client, wl_resource* resource, uint32_t id, wl_resource* toplevel) override
    {
        if (auto app = miral::application_for(client))
            puts("###################### app not null ######################");
        else
            puts("######################## app NULL ########################");

        new ToplevelDecorationV1{client, resource, id, toplevel};
    }
};

static_assert(
    mir::wayland::DecorationManagerV1::interface_version >= DecorationManagerV1::interface_supported,
    "Server decoration protocol newer than supported version");

int const DecorationManagerV1::interface_supported;
}

miral::WaylandExtensions::Builder const mir::examples::xdg_decoration_extension{
    DecorationManagerV1::interface_name,
    [](miral::WaylandExtensions::Context const* context)
        {
            return std::make_shared<DecorationManagerV1>(context->display(), DecorationManagerV1::interface_supported);
        }
};
