/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by:
 *   Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "render_surface.h"
#include "buffer_stream.h"
#include "mir_wait_handle.h"
#include "rpc/mir_display_server.h"

#include "mir/client/client_platform.h"

#include <boost/throw_exception.hpp>

namespace mcl = mir::client;
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirBufferStream* mcl::RenderSurface::get_buffer_stream(
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage buffer_usage)
{
    if (chain_from_id || stream_from_id)
        BOOST_THROW_EXCEPTION(std::logic_error("Content already handed out"));

    protobuf_bs->set_pixel_format(format);
    protobuf_bs->set_buffer_usage(buffer_usage);
    stream_from_id = connection_->create_client_buffer_stream_with_id(width,
                                                                      height,
                                                                      this,
                                                                      *protobuf_bs);
    if (buffer_usage == mir_buffer_usage_hardware)
    {
#pragma GCC diagnostic pop
        platform->use_egl_native_window(
            wrapped_native_window, dynamic_cast<EGLNativeSurface*>(stream_from_id.get()));
    }

    return stream_from_id.get();
}

MirPresentationChain* mcl::RenderSurface::get_presentation_chain()
{
    if (chain_from_id || stream_from_id)
        BOOST_THROW_EXCEPTION(std::logic_error("Content already handed out"));

    chain_from_id = connection_->create_presentation_chain_with_id(this,
                                                                   *protobuf_bs);
    //TODO: Figure out how to handle mir_buffer_usage_hardware once
    //      EGL is made to support RSs.

    return reinterpret_cast<MirPresentationChain*>(chain_from_id.get());
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
