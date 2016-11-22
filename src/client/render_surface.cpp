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
namespace mp = mir::protobuf;
namespace mf = mir::frontend;

mcl::RenderSurface::RenderSurface(
    MirConnection* const connection,
    std::shared_ptr<void> native_window,
    std::shared_ptr<ClientPlatform> client_platform,
    std::shared_ptr<mp::BufferStream> protobuf_bs,
    geom::Size size) :
    connection_(connection),
    wrapped_native_window(native_window),
    platform(client_platform),
    protobuf_bs(protobuf_bs),
    desired_size{size}
{
}

MirConnection* mcl::RenderSurface::connection() const
{
    return connection_;
}

mf::BufferStreamId mcl::RenderSurface::stream_id() const
{
    return mf::BufferStreamId(protobuf_bs->id().value());
}

MirBufferStream* mcl::RenderSurface::get_buffer_stream(
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage buffer_usage)
{
    if (!stream_from_id)
    {
        protobuf_bs->set_pixel_format(format);
        protobuf_bs->set_buffer_usage(buffer_usage);
        stream_from_id = connection_->create_client_buffer_stream_with_id(width,
                                                                          height,
                                                                          this,
                                                                          *protobuf_bs);
        if (buffer_usage == mir_buffer_usage_hardware)
        {
            platform->use_egl_native_window(
                wrapped_native_window, dynamic_cast<EGLNativeSurface*>(stream_from_id.get()));
        }
    }

    return reinterpret_cast<MirBufferStream*>(
        dynamic_cast<ClientBufferStream*>(stream_from_id.get()));
}

geom::Size mcl::RenderSurface::size() const
{
    std::lock_guard<decltype(size_mutex)> lk(size_mutex);
    return desired_size;
}

void mcl::RenderSurface::set_size(mir::geometry::Size size)
{
    std::lock_guard<decltype(size_mutex)> lk(size_mutex);
    desired_size = size;
}

bool mcl::RenderSurface::valid() const
{
    return protobuf_bs->has_id() && !protobuf_bs->has_error();
}

char const* mcl::RenderSurface::get_error_message() const
{
    if (protobuf_bs->has_error())
    {
        return protobuf_bs->error().c_str();
    }
    return "";
}
