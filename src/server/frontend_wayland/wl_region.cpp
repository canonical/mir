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

#include <algorithm>

namespace mf = mir::frontend;
namespace geom = mir::geometry;
namespace mw = mir::wayland;

mf::WlRegion::WlRegion(wl_resource* new_resource)
    : mw::Region(new_resource, Version<1>())
{}

mf::WlRegion::~WlRegion()
{}

std::vector<geom::Rectangle> mf::WlRegion::rectangle_vector()
{
    return rects;
}

mf::WlRegion* mf::WlRegion::from(wl_resource* resource)
{
    void* raw = wl_resource_get_user_data(resource);
    return static_cast<WlRegion*>(static_cast<wayland::Region*>(raw));
}

void mf::WlRegion::add(int32_t x, int32_t y, int32_t width, int32_t height)
{
    geom::Rectangle const rect{{x, y}, {width, height}};

    // A degenerate rectangle adds no area.
    if (rect.size.width.as_int() <= 0 || rect.size.height.as_int() <= 0)
        return;

    // Any existing rectangle wholly covered by the new one is now redundant.
    std::erase_if(rects, [&](geom::Rectangle const& existing) { return rect.contains(existing); });

    rects.push_back(rect);
}

void mf::WlRegion::subtract(int32_t x, int32_t y, int32_t width, int32_t height)
{
    geom::Rectangle const hole{{x, y}, {width, height}};

    // A degenerate hole subtracts nothing.
    if (hole.size.width.as_int() <= 0 || hole.size.height.as_int() <= 0)
        return;

    std::vector<geom::Rectangle> result;
    result.reserve(rects.size());

    for (auto const& rect : rects)
    {
        if (!rect.overlaps(hole))
        {
            result.push_back(rect);
            continue;
        }

        auto const rx1 = rect.left().as_int();
        auto const rx2 = rect.right().as_int();
        auto const ry1 = rect.top().as_int();
        auto const ry2 = rect.bottom().as_int();

        // The portion of the hole that actually intersects this rectangle.
        auto const hx1 = std::max(rx1, hole.left().as_int());
        auto const hx2 = std::min(rx2, hole.right().as_int());
        auto const hy1 = std::max(ry1, hole.top().as_int());
        auto const hy2 = std::min(ry2, hole.bottom().as_int());

        // Decompose rect minus hole into up to four non-overlapping slices.
        if (ry1 < hy1) // above the hole
            result.push_back(geom::Rectangle{{rx1, ry1}, {rx2 - rx1, hy1 - ry1}});
        if (hy2 < ry2) // below the hole
            result.push_back(geom::Rectangle{{rx1, hy2}, {rx2 - rx1, ry2 - hy2}});
        if (rx1 < hx1) // left of the hole
            result.push_back(geom::Rectangle{{rx1, hy1}, {hx1 - rx1, hy2 - hy1}});
        if (hx2 < rx2) // right of the hole
            result.push_back(geom::Rectangle{{hx2, hy1}, {rx2 - hx2, hy2 - hy1}});
    }

    rects = std::move(result);
}
