/*
 * Copyright © 2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "xdg_toplevel_v6.h"

#include "xdg_surface_v6.h"

#include "mir/geometry/size.h"

namespace mf = mir::frontend;
namespace geom = mir::geometry;

mf::XdgToplevelV6::XdgToplevelV6(struct wl_client* client, struct wl_resource* parent, uint32_t id,
                             std::shared_ptr<mf::Shell> const& shell, XdgSurfaceV6* self)
    : wayland::XdgToplevelV6(client, parent, id),
      shell{shell},
      self{self}
{
    self->set_notify_resize(
        [this](geom::Size const& new_size)
        {
            wl_array states;
            wl_array_init(&states);

            zxdg_toplevel_v6_send_configure(resource, new_size.width.as_int(), new_size.height.as_int(), &states);
        });
}

void mf::XdgToplevelV6::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgToplevelV6::set_parent(std::experimental::optional<struct wl_resource*> const& parent)
{
    if (parent && parent.value())
    {
        self->set_parent(get_xdgtoplevel(parent.value())->self->surface_id);
    }
    else
    {
        self->set_parent({});
    }

}

void mf::XdgToplevelV6::set_title(std::string const& title)
{
    self->set_title(title);
}

void mf::XdgToplevelV6::set_app_id(std::string const& /*app_id*/)
{
    // Logically this sets the session name, but Mir doesn't allow this (currently) and
    // allowing e.g. "session_for_client(client)->name(app_id);" would break the libmirserver ABI
}

void mf::XdgToplevelV6::show_window_menu(struct wl_resource* seat, uint32_t serial, int32_t x, int32_t y)
{
    (void)seat, (void)serial, (void)x, (void)y;
    // TODO
}

void mf::XdgToplevelV6::move(struct wl_resource* seat, uint32_t serial)
{
    self->move(seat, serial);
}

void mf::XdgToplevelV6::resize(struct wl_resource* seat, uint32_t serial, uint32_t edges)
{
    self->resize(seat, serial, edges);
}

void mf::XdgToplevelV6::set_max_size(int32_t width, int32_t height)
{
    self->set_max_size(width, height);
}

void mf::XdgToplevelV6::set_min_size(int32_t width, int32_t height)
{
    self->set_min_size(width, height);
}

void mf::XdgToplevelV6::set_maximized()
{
    self->set_maximized();
}

void mf::XdgToplevelV6::unset_maximized()
{
    self->unset_maximized();
}

void mf::XdgToplevelV6::set_fullscreen(std::experimental::optional<struct wl_resource*> const& output)
{
    (void)output;
    // TODO
}

void mf::XdgToplevelV6::unset_fullscreen()
{
    // TODO
}

void mf::XdgToplevelV6::set_minimized()
{
    // TODO
}

mf::XdgToplevelV6* mf::XdgToplevelV6::get_xdgtoplevel(wl_resource* surface) const
{
    auto* tmp = wl_resource_get_user_data(surface);
    return static_cast<XdgToplevelV6*>(static_cast<wayland::XdgToplevelV6*>(tmp));
}
