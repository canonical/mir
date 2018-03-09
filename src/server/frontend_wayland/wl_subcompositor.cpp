/*
 * Copyright © 2015-2018 Canonical Ltd.
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

#include "wl_subcompositor.h"

#include "wl_surface.h"

namespace mf = mir::frontend;

mf::WlSubcompositor::WlSubcompositor(struct wl_display* display)
    : wayland::Subcompositor(display, 1)
{}

void mf::WlSubcompositor::destroy(struct wl_client* /*client*/, struct wl_resource* resource)
{
    wl_resource_destroy(resource);
}

void mf::WlSubcompositor::get_subsurface(struct wl_client* client, struct wl_resource* resource, uint32_t id,
                                         struct wl_resource* surface, struct wl_resource* parent)
{
    new WlSubsurface(client, resource, id, WlSurface::from(surface), WlSurface::from(parent));
}

mf::WlSubsurface::WlSubsurface(struct wl_client* client, struct wl_resource* object_parent, uint32_t id, WlSurface* surface, WlSurface* parent_surface)
    : wayland::Subsurface(client, object_parent, id),
      surface{surface},
      parent{parent_surface->add_child(this)}
{
    surface->set_role(this);
}

mf::WlSubsurface::~WlSubsurface()
{
    // unique pointer automatically removes `this` from parent child list

    invalidate_buffer_list();
    surface->set_role(null_wl_mir_window_ptr);
}

void mf::WlSubsurface::populate_buffer_list(std::vector<shell::StreamSpecification>& buffers) const
{
    surface->populate_buffer_list(buffers);
}

void mf::WlSubsurface::set_position(int32_t x, int32_t y)
{
    (void)x; (void)y;
    // TODO
}

void mf::WlSubsurface::place_above(struct wl_resource* sibling)
{
    (void)sibling;
    // TODO
}

void mf::WlSubsurface::place_below(struct wl_resource* sibling)
{
    (void)sibling;
    // TODO
}

void mf::WlSubsurface::set_sync()
{
    // TODO
}

void mf::WlSubsurface::set_desync()
{
    // TODO
}

void mf::WlSubsurface::destroy()
{
    wl_resource_destroy(resource);
}

void mf::WlSubsurface::new_buffer_size(geometry::Size const& buffer_size)
{
    (void)buffer_size;
    // TODO
}

void mf::WlSubsurface::invalidate_buffer_list()
{
    parent->invalidate_buffer_list();
}

void mf::WlSubsurface::commit()
{
    invalidate_buffer_list();
    // TODO: if in desync mode, immediately make the buffer get rendered
}

void mf::WlSubsurface::visiblity(bool visible)
{
    (void)visible;
    // TODO
}
