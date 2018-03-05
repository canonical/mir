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
 *              William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_WL_SUBSURFACE_H
#define MIR_FRONTEND_WL_SUBSURFACE_H

#include "generated/wayland_wrapper.h"

namespace mir
{
namespace frontend
{

class WlSubcompositor: wayland::Subcompositor
{
public:
    WlSubcompositor(struct wl_display* display);

private:
    void destroy(struct wl_client* client, struct wl_resource* resource) override;
    void get_subsurface(struct wl_client* client, struct wl_resource* resource, uint32_t id,
                        struct wl_resource* surface, struct wl_resource* parent) override;
};

}
}

#endif // MIR_FRONTEND_WL_SUBSURFACE_H
