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
    mf::BufferStreamId stream_id, std::function<void(ClientBufferStream*)> const& exec) const
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
        auto const cit = contexts.find(stream_id);
        if (cit != contexts.end())
        {
            struct Wrapper : ClientBufferStream
            {
                Wrapper(std::shared_ptr<mcl::PresentationChain> context) : context(context) {}
                MirSurfaceParameters get_parameters() const
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                std::shared_ptr<ClientBuffer> get_current_buffer()
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                uint32_t get_current_buffer_id()
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                EGLNativeWindowType egl_native_window()
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                MirWaitHandle* next_buffer(std::function<void()> const&)
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                std::shared_ptr<MemoryRegion> secure_for_cpu_write()
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                int swap_interval() const
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                MirWaitHandle* set_swap_interval(int)
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                MirNativeBuffer* get_current_buffer_package()
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                MirPlatformType platform_type()
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                frontend::BufferStreamId rpc_id() const
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                bool valid() const
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                void buffer_unavailable()
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                void set_size(geometry::Size)
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                MirWaitHandle* set_scale(float)
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                char const* get_error_message() const
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }
                MirConnection* connection() const
                { BOOST_THROW_EXCEPTION(std::runtime_error("NO")); }

                void buffer_available(mir::protobuf::Buffer const& b)
                    { context->buffer_available(b); }
                std::shared_ptr<mcl::PresentationChain> const context;
            } w(cit->second);
            exec(&w);
            return;
        }

        std::stringstream ss;
        ss << __PRETTY_FUNCTION__ << "executed with non-existent stream ID " << stream_id;
        BOOST_THROW_EXCEPTION(std::runtime_error(ss.str()));
    }
}

void mcl::ConnectionSurfaceMap::with_all_streams_do(std::function<void(ClientBufferStream*)> const& fn) const
{
    std::shared_lock<decltype(guard)> lk(guard);
    for(auto const& stream : streams)
        fn(stream.second.get());
}

void mcl::ConnectionSurfaceMap::insert(
    mf::BufferStreamId stream_id, std::shared_ptr<PresentationChain> const& context)
{
    std::lock_guard<decltype(guard)> lk(guard);
    contexts[stream_id] = context;
}

void mcl::ConnectionSurfaceMap::insert(
    mf::BufferStreamId stream_id, std::shared_ptr<ClientBufferStream> const& stream)
{
    std::lock_guard<decltype(guard)> lk(guard);
    streams[stream_id] = stream;
}

void mcl::ConnectionSurfaceMap::erase(mf::BufferStreamId stream_id)
{
    std::lock_guard<decltype(guard)> lk(guard);
    streams.erase(stream_id);
}
