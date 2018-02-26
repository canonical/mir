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

#include "xdg_positioner_v6.h"

namespace mf = mir::frontend;
namespace geom = mir::geometry;

mf::XdgPositionerV6::XdgPositionerV6(struct wl_client* client, struct wl_resource* parent, uint32_t id)
    : wayland::XdgPositionerV6(client, parent, id)
{}

void mf::XdgPositionerV6::destroy()
{
    wl_resource_destroy(resource);
}

void mf::XdgPositionerV6::set_size(int32_t width, int32_t height)
{
    size = geom::Size{width, height};
}

void mf::XdgPositionerV6::set_anchor_rect(int32_t x, int32_t y, int32_t width, int32_t height)
{
    aux_rect = geom::Rectangle{{x, y}, {width, height}};
}

void mf::XdgPositionerV6::set_anchor(uint32_t anchor)
{
    MirPlacementGravity placement = mir_placement_gravity_center;

    if (anchor & ZXDG_POSITIONER_V6_ANCHOR_TOP)
        placement = MirPlacementGravity(placement | mir_placement_gravity_north);

    if (anchor & ZXDG_POSITIONER_V6_ANCHOR_BOTTOM)
        placement = MirPlacementGravity(placement | mir_placement_gravity_south);

    if (anchor & ZXDG_POSITIONER_V6_ANCHOR_LEFT)
        placement = MirPlacementGravity(placement | mir_placement_gravity_west);

    if (anchor & ZXDG_POSITIONER_V6_ANCHOR_RIGHT)
        placement = MirPlacementGravity(placement | mir_placement_gravity_east);

    surface_placement_gravity = placement;
}

void mf::XdgPositionerV6::set_gravity(uint32_t gravity)
{
    MirPlacementGravity placement = mir_placement_gravity_center;

    if (gravity & ZXDG_POSITIONER_V6_GRAVITY_TOP)
        placement = MirPlacementGravity(placement | mir_placement_gravity_north);

    if (gravity & ZXDG_POSITIONER_V6_GRAVITY_BOTTOM)
        placement = MirPlacementGravity(placement | mir_placement_gravity_south);

    if (gravity & ZXDG_POSITIONER_V6_GRAVITY_LEFT)
        placement = MirPlacementGravity(placement | mir_placement_gravity_west);

    if (gravity & ZXDG_POSITIONER_V6_GRAVITY_RIGHT)
        placement = MirPlacementGravity(placement | mir_placement_gravity_east);

    aux_rect_placement_gravity = placement;
}

void mf::XdgPositionerV6::set_constraint_adjustment(uint32_t constraint_adjustment)
{
    (void)constraint_adjustment;
    // TODO
}

void mf::XdgPositionerV6::set_offset(int32_t x, int32_t y)
{
    aux_rect_placement_offset_x = x;
    aux_rect_placement_offset_y = y;
}
