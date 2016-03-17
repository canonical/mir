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
#include "presentation_chain.h"

#include <boost/throw_exception.hpp>
#include <sstream>

namespace mcl=mir::client;
namespace mf=mir::frontend;

void mcl::ConnectionSurfaceMap::with_surface_do(
    mf::SurfaceId surface_id, std::function<void(MirSurface*)> const& exec) const
{
    std::shared_lock<decltype(guard)> lk(guard);
    auto const it = surfaces.find(surface_id);
    if (it != surfaces.end())
    {
        auto const surface = it->second;
        exec(surface.get());
    }
    else
    {
        std::stringstream ss;
        ss << __PRETTY_FUNCTION__ << "executed with non-existent surface ID " << surface_id;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
}

void mcl::ConnectionSurfaceMap::insert(mf::SurfaceId surface_id, std::shared_ptr<MirSurface> const& surface)
{
    std::lock_guard<decltype(guard)> lk(guard);
    surfaces[surface_id] = surface;
}

void mcl::ConnectionSurfaceMap::erase(mf::SurfaceId surface_id)
{
    std::lock_guard<decltype(guard)> lk(guard);
    surfaces.erase(surface_id);
}

void mcl::ConnectionSurfaceMap::with_stream_do(
    mf::BufferStreamId stream_id, std::function<void(BufferReceiver*)> const& exec) const
{
    std::shared_lock<decltype(guard)> lk(guard);
    auto const it = streams.find(stream_id);
    if (it != streams.end())
    {
        auto const stream = it->second;
        exec(stream.get());
    }
    else
    {
        std::stringstream ss;
        ss << __PRETTY_FUNCTION__ << "executed with non-existent stream ID " << stream_id;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
}

void mcl::ConnectionSurfaceMap::with_all_streams_do(std::function<void(BufferReceiver*)> const& fn) const
{
    std::shared_lock<decltype(guard)> lk(guard);
    for(auto const& stream : streams)
        fn(stream.second.get());
}

void mcl::ConnectionSurfaceMap::insert(
    mf::BufferStreamId stream_id, std::shared_ptr<BufferReceiver> const& stream)
{
    std::lock_guard<decltype(guard)> lk(guard);
    streams[stream_id] = stream;
}

void mcl::ConnectionSurfaceMap::erase(mf::BufferStreamId stream_id)
{
    std::lock_guard<decltype(guard)> lk(guard);
    streams.erase(stream_id);
}

void mcl::ConnectionSurfaceMap::insert(int buffer_id, std::shared_ptr<mcl::Buffer> const& buffer)
{
    std::lock_guard<decltype(guard)> lk(guard);
    buffers[buffer_id] = buffer;
}

void mcl::ConnectionSurfaceMap::erase(int buffer_id)
{
    std::lock_guard<decltype(guard)> lk(guard);
    buffers.erase(buffer_id);
}

std::shared_ptr<mcl::Buffer> mcl::ConnectionSurfaceMap::buffer(int buffer_id) const
{
    std::lock_guard<decltype(guard)> lk(guard);
    auto const it = buffers.find(buffer_id);
    if (it != buffers.end())
        return it->second;
    return nullptr;
}
