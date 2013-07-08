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

geom::Rectangle ms::SurfaceDataStorage::size_and_position() const
{
    std::unique_lock<std::mutex> lk(guard);
    return geom::Rectangle{surface_top_left, surface_size};
}

std::string const& ms::SurfaceDataStorage::name() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_name;
}
void ms::SurfaceDataStorage::move_to(geom::Point new_pt)
{
    std::unique_lock<std::mutex> lk(guard);
    surface_top_left = new_pt;
}
