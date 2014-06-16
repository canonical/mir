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
#include "mir_surface.h"

#include <boost/throw_exception.hpp>
#include <sstream>

namespace mcl=mir::client;

mcl::ConnectionSurfaceMap::ConnectionSurfaceMap()
{
}

mcl::ConnectionSurfaceMap::~ConnectionSurfaceMap() noexcept
{
    // Unless the client has screwed up there should be no surfaces left
    // here. (OTOH *we* don't need to leak memory when clients screw up.)
    std::lock_guard<std::mutex> lk(guard);

    for (auto const& surface :surfaces)
    {
        if (MirSurface::is_valid(surface.second))
            delete surface.second;
    }
}

void mcl::ConnectionSurfaceMap::with_surface_do(
    int surface_id, std::function<void(MirSurface*)> exec) const
{
    std::lock_guard<std::mutex> lk(guard);
    auto const it = surfaces.find(surface_id);
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

void mcl::ConnectionSurfaceMap::insert(int surface_id, MirSurface* surface)
{
    std::lock_guard<std::mutex> lk(guard);
    surfaces[surface_id] = surface;
}

void mcl::ConnectionSurfaceMap::erase(int surface_id)
{
    std::lock_guard<std::mutex> lk(guard);
    surfaces.erase(surface_id);
}
