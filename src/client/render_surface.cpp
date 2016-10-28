/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by:
 *   Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "render_surface.h"
#include "buffer_stream.h"
#include "mir_wait_handle.h"
#include "rpc/mir_display_server.h"

#include "mir/client_platform.h"

#include <boost/throw_exception.hpp>

namespace mcl = mir::client;
namespace mclr = mcl::rpc;
namespace geom = mir::geometry;

namespace mir
{
namespace client
{
void render_surface_buffer_stream_create_callback(BufferStream* stream, void* context)
{
    RenderSurface::StreamCreationRequest* request{
        reinterpret_cast<RenderSurface::StreamCreationRequest*>(context)};
    RenderSurface* rs = request->rs;

    if (request->usage == mir_buffer_usage_hardware)
    {
        rs->platform->use_egl_native_window(
            rs->wrapped_native_window, dynamic_cast<EGLNativeSurface*>(stream));
    }

    if (request->callback)
        request->callback(
            reinterpret_cast<MirBufferStream*>(dynamic_cast<ClientBufferStream*>(stream)),
            request->context);

    {
        std::shared_lock<decltype(rs->guard)> lk(rs->guard);
        rs->stream_ = dynamic_cast<ClientBufferStream*>(stream);
        rs->stream_creation_request.reset();
    }
}

void render_surface_buffer_stream_release_callback(MirBufferStream* stream, void* context)
{
    RenderSurface::StreamReleaseRequest* request{
        reinterpret_cast<RenderSurface::StreamReleaseRequest*>(context)};
    RenderSurface* rs = request->rs;

    if (request->callback)
        request->callback(stream, request->context);

    {
        std::shared_lock<decltype(rs->guard)> lk(rs->guard);
        rs->stream_release_request.reset();
        rs->stream_ = nullptr;
    }
}
}
}

mcl::RenderSurface::RenderSurface(
    MirConnection* const connection,
    std::shared_ptr<void> native_window,
    std::shared_ptr<ClientPlatform> client_platform,
    geom::Size logical_size) :
    connection_(connection),
    wrapped_native_window(native_window),
    platform(client_platform),
    stream_(nullptr),
    stream_creation_request(nullptr),
    stream_release_request(nullptr),
    desired_logical_size{logical_size}
{
}

MirConnection* mcl::RenderSurface::connection() const
{
    return connection_;
}

mir::frontend::BufferStreamId mcl::RenderSurface::stream_id() const
{
    if (stream_)
        return stream_->rpc_id();
    else
        return mir::frontend::BufferStreamId{-1};
}

MirWaitHandle* mcl::RenderSurface::create_buffer_stream(
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage usage,
    mir_buffer_stream_callback callback,
    void* context)
{
    {
        std::shared_lock<decltype(guard)> lk(guard);

        if (stream_)
        {
            BOOST_THROW_EXCEPTION(
                std::logic_error("Render surface already has content"));
        }

        if (!stream_creation_request)
        {
            stream_creation_request =
                std::make_shared<StreamCreationRequest>(this, usage, callback, context);
        }
        else
        {
            BOOST_THROW_EXCEPTION(
                std::logic_error("Content in process of being created"));
        }
    }

    return connection_->create_client_buffer_stream(
        width, height, format, usage, this, nullptr,
        render_surface_buffer_stream_create_callback,
        stream_creation_request.get());
}

MirWaitHandle* mcl::RenderSurface::release_buffer_stream(
    mir_buffer_stream_callback callback,
    void* context)
{
    {
        std::shared_lock<decltype(guard)> lk(guard);

        if (!stream_)
        {
            BOOST_THROW_EXCEPTION(
                std::logic_error("Render surface has no content"));
        }

        if (!stream_release_request)
        {
            stream_release_request =
                std::make_shared<StreamReleaseRequest>(this, callback, context);
        }
        else
        {
            BOOST_THROW_EXCEPTION(
                std::logic_error("Content in process of being released"));
        }
    }

    return connection_->release_buffer_stream(
        stream_, render_surface_buffer_stream_release_callback,
        stream_release_request.get());
}

geom::Size mcl::RenderSurface::logical_size() const
{
    std::lock_guard<decltype(size_mutex)> lk(size_mutex);
    return desired_logical_size;
}

void mcl::RenderSurface::set_logical_size(mir::geometry::Size logical_size)
{
    std::lock_guard<decltype(size_mutex)> lk(size_mutex);
    desired_logical_size = logical_size;
}
