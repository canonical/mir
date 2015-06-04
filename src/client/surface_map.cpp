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
namespace mf=mir::frontend;

mcl::ConnectionSurfaceMap::ConnectionSurfaceMap()
{
}

mcl::ConnectionSurfaceMap::~ConnectionSurfaceMap() noexcept
{
    // Unless the client has screwed up there should be no surfaces left
    // here. (OTOH *we* don't need to leak memory when clients screw up.)
    std::lock_guard<std::mutex> lk(guard);

    for (auto const& surface : surfaces)
    {
        if (MirSurface::is_valid(surface.second))
            delete surface.second;
    }

    for (auto const& stream : streams)
    {
        //don't delete streams that were managed by the surface
        if (std::get<1>(stream.second))
            delete std::get<0>(stream.second);
    }
}

void mcl::ConnectionSurfaceMap::with_surface_do(
    mf::SurfaceId surface_id, std::function<void(MirSurface*)> const& exec) const
{
    std::lock_guard<std::mutex> lk(guard);
    auto const it = surfaces.find(surface_id);
    if (it != surfaces.end())
    {
        exec(it->second);
    }
    else
    {
        std::stringstream ss;
        ss << __PRETTY_FUNCTION__ << "executed with non-existent surface ID " << surface_id;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
}

void mcl::ConnectionSurfaceMap::insert(mf::SurfaceId surface_id, MirSurface* surface)
{
    std::lock_guard<std::mutex> lk(guard);
    surfaces[surface_id] = surface;
    streams[mf::BufferStreamId(surface_id.as_value())] = std::make_tuple(surface->get_buffer_stream(), false);
}

void mcl::ConnectionSurfaceMap::erase(mf::SurfaceId surface_id)
{
    std::lock_guard<std::mutex> lk(guard);
    surfaces.erase(surface_id);
}

void mcl::ConnectionSurfaceMap::with_stream_do(
    mf::BufferStreamId stream_id, std::function<void(ClientBufferStream*)> const& exec) const
{
    std::lock_guard<std::mutex> lk(guard);
    auto const it = streams.find(stream_id);
    if (it != streams.end())
    {
        exec(std::get<0>(it->second));
    }
    else
    {
        std::stringstream ss;
        ss << __PRETTY_FUNCTION__ << "executed with non-existent stream ID " << stream_id;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
}

void mcl::ConnectionSurfaceMap::insert(mf::BufferStreamId stream_id, ClientBufferStream* stream)
{
    std::lock_guard<std::mutex> lk(guard);
    streams[stream_id] = std::make_tuple(stream, true);
}

void mcl::ConnectionSurfaceMap::erase(mf::BufferStreamId stream_id)
{
    std::lock_guard<std::mutex> lk(guard);
    streams.erase(stream_id);
}
