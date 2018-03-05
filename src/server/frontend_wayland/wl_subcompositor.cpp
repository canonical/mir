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

namespace mf = mir::frontend;

mf::WlSubcompositor::WlSubcompositor(struct wl_display* display)
    : wayland::Subcompositor(display, 1)
{

}

void mf::WlSubcompositor::destroy(struct wl_client* /*client*/, struct wl_resource* resource)
{
    wl_resource_destroy(resource);
}

void mf::WlSubcompositor::get_subsurface(struct wl_client* client, struct wl_resource* resource, uint32_t id,
                                         struct wl_resource* surface, struct wl_resource* parent)
{
    (void)client; (void)resource; (void)resource; (void)id; (void)surface; (void)parent;
    mir::log_critical("oh shit, they want a subsurface. wtf do I do?");
}
