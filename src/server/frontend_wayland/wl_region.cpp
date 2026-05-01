/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "wl_region.h"

#include <mir/log.h>

namespace mf = mir::frontend;
namespace geom = mir::geometry;
namespace mw = mir::wayland_rs;

std::vector<geom::Rectangle> mf::WlRegion::rectangle_vector()
{
    return rects;
}

void mf::WlRegion::add(int32_t x, int32_t y, int32_t width, int32_t height)
{
    rects.push_back(geom::Rectangle{{x, y}, {width, height}});
}

void mf::WlRegion::subtract(int32_t x, int32_t y, int32_t width, int32_t height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    log_warning("WlRegion::subtract not implemented. ignoring.");
}

mir::frontend::WlRegion* mf::WlRegion::from(WlRegionImpl* region)
{
    return dynamic_cast<WlRegion*>(region);
}
