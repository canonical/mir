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

namespace
{
struct ToplevelDecorationV1 : mir::wayland::ToplevelDecorationV1
{
    ToplevelDecorationV1(struct wl_client *client, struct wl_resource *parent, uint32_t id) :
        mir::wayland::ToplevelDecorationV1::ToplevelDecorationV1(client, parent, id)
    {
        send_configure_event(mode);
    }

    void destroy() override
    {
        destroy_wayland_object();
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
};

struct DecorationManagerV1 : mir::wayland::DecorationManagerV1
{
    static int const interface_supported = 1;

    using mir::wayland::DecorationManagerV1::DecorationManagerV1;

    void destroy(struct wl_client* /*client*/, struct wl_resource* resource) override
    {
        destroy_wayland_object(resource);
    }

    void get_toplevel_decoration(wl_client* client, wl_resource* resource, uint32_t id, wl_resource* /*toplevel*/) override
    {
        // TODO: we need a way to map `client` to a miral::Application
        // TODO: we need a way to map `toplevel` to a miral::Surface
        new ToplevelDecorationV1{client, resource, id};
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
