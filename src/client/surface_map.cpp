/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "connection_surface_map.h"
#include <sstream>
#include <boost/throw_exception.hpp>

namespace mcl=mir::client;

mcl::ConnectionSurfaceMap::ConnectionSurfaceMap()
{
}

void mcl::ConnectionSurfaceMap::with_surface_do(
    int const& surface_id, std::function<void(MirSurface*)> exec)
{
    std::unique_lock<std::mutex> lk(guard);
    SurfaceMap::iterator it = surfaces.find(surface_id);
    if (it != surfaces.end())
    {
        MirSurface *surface = it->second;
        exec(surface);
    }
    else
    {
        std::stringstream ss;
        ss << __PRETTY_FUNCTION__
           << "executed with non-existent surface ID "
           << surface_id << ".\n";

        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
}

void mcl::ConnectionSurfaceMap::insert(int const& surface_id, MirSurface* surface)
{
    std::unique_lock<std::mutex> lk(guard);
    surfaces[surface_id] = surface;
}

void mcl::ConnectionSurfaceMap::erase(int surface_id)
{
    std::unique_lock<std::mutex> lk(guard);
    surfaces.erase(surface_id);
}
