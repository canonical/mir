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
    std::unordered_map<frontend::SurfaceId, MirSurface*> surface_map;
    std::unordered_map<frontend::BufferStreamId, StreamInfo> stream_map;
    {
        //Prevent TSAN from flagging lock ordering issues
        //as the surface/buffer_stream destructors acquire internal locks
        //The mutex lock is used mainly as a memory barrier here
        std::lock_guard<std::mutex> lk(guard);
        surface_map = std::move(surfaces);
        stream_map = std::move(streams);
    }

    // Unless the client has screwed up there should be no surfaces left
    // here. (OTOH *we* don't need to leak memory when clients screw up.)
    for (auto const& surface : surface_map)
    {
        if (MirSurface::is_valid(surface.second))
            delete surface.second;
    }

    for (auto const& info : stream_map)
    {
        if (info.second.owned)
            delete info.second.stream;
    }
}

void mcl::ConnectionSurfaceMap::with_surface_do(
    mf::SurfaceId surface_id, std::function<void(MirSurface*)> const& exec) const
{
    std::unique_lock<std::mutex> lk(guard);
    auto const it = surfaces.find(surface_id);
    if (it != surfaces.end())
    {
        auto const surface = it->second;
        lk.unlock();
        exec(surface);
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
    // get_buffer_stream has internal locks - call before locking mutex to
    // avoid locking ordering issues
    auto const stream = surface->get_buffer_stream();
    std::lock_guard<std::mutex> lk(guard);
    surfaces[surface_id] = surface;
    streams[mf::BufferStreamId(surface_id.as_value())] = {stream, false};
}

void mcl::ConnectionSurfaceMap::erase(mf::SurfaceId surface_id)
{
    std::lock_guard<std::mutex> lk(guard);
    surfaces.erase(surface_id);
    streams.erase(mf::BufferStreamId(surface_id.as_value()));
}

void mcl::ConnectionSurfaceMap::with_stream_do(
    mf::BufferStreamId stream_id, std::function<void(ClientBufferStream*)> const& exec) const
{
    std::unique_lock<std::mutex> lk(guard);
    auto const it = streams.find(stream_id);
    if (it != streams.end())
    {
        auto const stream = it->second.stream;
        lk.unlock();
        exec(stream);
    }
    else
    {
        std::stringstream ss;
        ss << __PRETTY_FUNCTION__ << "executed with non-existent stream ID " << stream_id;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
}

void mcl::ConnectionSurfaceMap::with_all_streams_do(std::function<void(ClientBufferStream*)> const& fn) const
{
    std::unique_lock<std::mutex> lk(guard);
    for(auto const& stream : streams)
        fn(stream.second.stream);
}

void mcl::ConnectionSurfaceMap::insert(mf::BufferStreamId stream_id, ClientBufferStream* stream)
{
    std::lock_guard<std::mutex> lk(guard);
    streams[stream_id] = {stream, true};
}

void mcl::ConnectionSurfaceMap::erase(mf::BufferStreamId stream_id)
{
    std::lock_guard<std::mutex> lk(guard);
    streams.erase(stream_id);
}
