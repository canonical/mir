/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

using mir::client::ConnectionSurfaceMap;
using mir::frontend::SurfaceId;
using mir::frontend::BufferStreamId;

std::shared_ptr<MirWindow> ConnectionSurfaceMap::surface(SurfaceId id) const
{
    std::shared_lock<decltype(guard)> lk(guard);
    auto const found = surfaces.find(id);
    return found != surfaces.end() ? found->second : nullptr;
}

void mcl::ConnectionSurfaceMap::insert(mf::SurfaceId surface_id, std::shared_ptr<MirWindow> const& surface)
{
    std::lock_guard<decltype(guard)> lk(guard);
    surfaces[surface_id] = surface;
}

void mcl::ConnectionSurfaceMap::erase(mf::SurfaceId surface_id)
{
    std::lock_guard<decltype(guard)> lk(guard);
    surfaces.erase(surface_id);
}

std::shared_ptr<MirBufferStream> ConnectionSurfaceMap::stream(BufferStreamId id) const
{
    std::shared_lock<decltype(stream_guard)> lk(stream_guard);
    auto const found = streams.find(id);
    return found != streams.end() ? found->second : nullptr;
}

void mcl::ConnectionSurfaceMap::with_all_streams_do(std::function<void(MirBufferStream*)> const& fn) const
{
    std::shared_lock<decltype(stream_guard)> lk(stream_guard);
    for(auto const& stream : streams)
        fn(stream.second.get());
}

void mcl::ConnectionSurfaceMap::insert(
    mf::BufferStreamId stream_id, std::shared_ptr<MirBufferStream> const& stream)
{
    std::lock_guard<decltype(stream_guard)> lk(stream_guard);
    streams[stream_id] = stream;
}

void mcl::ConnectionSurfaceMap::insert(
    mf::BufferStreamId stream_id, std::shared_ptr<MirPresentationChain> const& chain)
{
    std::lock_guard<decltype(stream_guard)> lk(stream_guard);
    chains[stream_id] = chain; 
}

void mcl::ConnectionSurfaceMap::erase(mf::BufferStreamId stream_id)
{
    std::lock_guard<decltype(stream_guard)> lk(stream_guard);
    auto stream_it = streams.find(stream_id);
    auto chain_it = chains.find(stream_id);
    if (stream_it != streams.end())
        streams.erase(stream_it);
    if (chain_it != chains.end())
        chains.erase(chain_it);
}

void mcl::ConnectionSurfaceMap::insert(int buffer_id, std::shared_ptr<mcl::MirBuffer> const& buffer)
{
    std::lock_guard<decltype(buffer_guard)> lk(buffer_guard);
    buffers[buffer_id] = buffer;
}

void mcl::ConnectionSurfaceMap::erase(int buffer_id)
{
    std::lock_guard<decltype(buffer_guard)> lk(buffer_guard);
    buffers.erase(buffer_id);
}

std::shared_ptr<mcl::MirBuffer> mcl::ConnectionSurfaceMap::buffer(int buffer_id) const
{
    std::shared_lock<decltype(buffer_guard)> lk(buffer_guard);
    auto const it = buffers.find(buffer_id);
    if (it != buffers.end())
        return it->second;
    else
        return nullptr;
}

void mcl::ConnectionSurfaceMap::erase(void* render_surface_key)
{
    std::lock_guard<decltype(guard)> lk(stream_guard);
    auto rs_it = render_surfaces.find(render_surface_key);
    if (rs_it != render_surfaces.end())
        render_surfaces.erase(rs_it);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void mcl::ConnectionSurfaceMap::insert(void* render_surface_key, std::shared_ptr<MirRenderSurface> const& render_surface)
{
    std::lock_guard<decltype(guard)> lk(stream_guard);
    render_surfaces[render_surface_key] = render_surface;
}

std::shared_ptr<MirRenderSurface> mcl::ConnectionSurfaceMap::render_surface(void* render_surface_key) const
{
    std::shared_lock<decltype(guard)> lk(stream_guard);
    auto const it = render_surfaces.find(render_surface_key);
    if (it != render_surfaces.end())
        return it->second;
    else
        BOOST_THROW_EXCEPTION(std::runtime_error("could not find render surface"));
}
#pragma GCC diagnostic pop

void mcl::ConnectionSurfaceMap::with_all_windows_do(std::function<void(MirWindow*)> const& fn) const
{
    std::shared_lock<decltype(stream_guard)> lk(guard);
    for(auto const& window : surfaces)
        fn(window.second.get());
}
