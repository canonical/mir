/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/surfaces/surface_data_storage.h"

namespace geom=mir::geometry;
namespace ms = mir::surfaces;

ms::SurfaceDataStorage::SurfaceDataStorage(
    std::string const& name, geom::Point top_left, geom::Size size)
    : surface_name(name),
      surface_top_left(top_left),
      surface_size(size)
{
}

geom::Point ms::SurfaceDataStorage::top_left() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_top_left;
}

geom::Size ms::SurfaceDataStorage::size() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_size;
}

std::string const& ms::SurfaceDataStorage::name() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_name;
}

void ms::SurfaceDataStorage::set_top_left(geom::Point new_pt)
{
    std::unique_lock<std::mutex> lk(guard);
    surface_top_left = new_pt;
}

#if 0
namespace
{

bool rectangle_contains_point(geom::Rectangle const& rectangle, uint32_t px, uint32_t py)
{
    auto width = rectangle.size.width.as_uint32_t();
    auto height = rectangle.size.height.as_uint32_t();
    auto x = rectangle.top_left.x.as_uint32_t();
    auto y = rectangle.top_left.y.as_uint32_t();
    
    if (px < x)
        return false;
    else if (py <  y)
        return false;
    else if (px > x + width)
        return false;
    else if (py > y + height)
        return false;
    return true;
}

}
#endif 
bool ms::SurfaceDataStorage::contains(geom::Point const& point) const
{
    (void) point;
#if 0
    for (auto const& rectangle : input_rectangles)
    {
        if (rectangle_contains_point(rectangle, point.x.as_uint32_t(), point.y.as_uint32_t()))
            return true;
    }
#endif
    return false;
}

void ms::SurfaceDataStorage::set_input_region(
    std::vector<geometry::Rectangle> const&)// input_rectangles);
{
}
