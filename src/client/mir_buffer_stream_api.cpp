/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#define MIR_LOG_COMPONENT "MirBufferStreamAPI"

#include "mir_screencast.h"
#include "mir_surface.h"
#include "mir_connection.h"
#include "buffer_stream.h"
#include "client_buffer_stream_factory.h"

#include "mir/client_buffer.h"

#include "mir/uncaught.h"

#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace mcl = mir::client;
namespace mp = mir::protobuf;

namespace
{
// assign_result is compatible with all 2-parameter callbacks
void assign_result(void* result, void** context)
{
    if (context)
        *context = result;
}

}



MirWaitHandle* mir_connection_create_buffer_stream(MirConnection *connection,
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage buffer_usage,
    mir_buffer_stream_callback callback,
    void *context)
try
{
    auto stream = connection->create_client_buffer_stream(
        width, height, format, buffer_usage, callback, context);
    return stream->get_create_wait_handle();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

MirBufferStream* mir_connection_create_buffer_stream_sync(MirConnection *connection,
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage buffer_usage)
try
{
    mcl::BufferStream *stream = nullptr;
    mir_connection_create_buffer_stream(connection, width, height, format, buffer_usage,
        reinterpret_cast<mir_buffer_stream_callback>(assign_result), &stream)->wait_for_all();
    return reinterpret_cast<MirBufferStream*>(dynamic_cast<mcl::ClientBufferStream*>(stream));
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

MirWaitHandle *mir_buffer_stream_release(
    MirBufferStream * buffer_stream,
    mir_buffer_stream_callback callback,
    void *context)
{
    auto *bs = reinterpret_cast<mcl::ClientBufferStream*>(buffer_stream);
    return bs->release(callback, context);
}

void mir_buffer_stream_release_sync(MirBufferStream *buffer_stream)
{
    mir_buffer_stream_release(buffer_stream, nullptr, nullptr)->wait_for_all();
}

void mir_buffer_stream_get_current_buffer(MirBufferStream* buffer_stream, MirNativeBuffer** buffer_package_out)
try
{
    mcl::ClientBufferStream *bs = reinterpret_cast<mcl::ClientBufferStream*>(buffer_stream);
    *buffer_package_out = bs->get_current_buffer_package();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

MirWaitHandle* mir_buffer_stream_swap_buffers(
    MirBufferStream* buffer_stream,
    mir_buffer_stream_callback callback,
    void* context)
try
{
    mcl::ClientBufferStream *bs = reinterpret_cast<mcl::ClientBufferStream*>(buffer_stream);
    return bs->next_buffer([bs, callback, context]{
            if (callback)
                callback(reinterpret_cast<MirBufferStream*>(bs), context);
    });
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

void mir_buffer_stream_swap_buffers_sync(MirBufferStream* buffer_stream)
{
    mir_wait_for(mir_buffer_stream_swap_buffers(buffer_stream,
        reinterpret_cast<mir_buffer_stream_callback>(assign_result),
        nullptr));
}

void mir_buffer_stream_get_graphics_region(
    MirBufferStream *buffer_stream,
    MirGraphicsRegion *region_out)
try
{
    mcl::ClientBufferStream *bs = reinterpret_cast<mcl::ClientBufferStream*>(buffer_stream);

    auto secured_region = bs->secure_for_cpu_write();
    region_out->width = secured_region->width.as_uint32_t();
    region_out->height = secured_region->height.as_uint32_t();
    region_out->stride = secured_region->stride.as_uint32_t();
    region_out->pixel_format = secured_region->format;
    region_out->vaddr = secured_region->vaddr.get();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

MirEGLNativeWindowType mir_buffer_stream_get_egl_native_window(MirBufferStream* buffer_stream)
try
{
    mcl::ClientBufferStream *bs = reinterpret_cast<mcl::ClientBufferStream*>(buffer_stream);
    return reinterpret_cast<MirEGLNativeWindowType>(bs->egl_native_window());
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return MirEGLNativeWindowType();
}

MirPlatformType mir_buffer_stream_get_platform_type(MirBufferStream* buffer_stream)
try
{
    mcl::ClientBufferStream *bs = reinterpret_cast<mcl::ClientBufferStream*>(buffer_stream);
    return bs->platform_type();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return MirPlatformType();
}

bool mir_buffer_stream_is_valid(MirBufferStream* opaque_stream)
{
    auto buffer_stream = reinterpret_cast<mcl::ClientBufferStream*>(opaque_stream);
    return buffer_stream->valid();
}

MirWaitHandle* mir_buffer_stream_set_scale(MirBufferStream* opaque_stream, float scale)
try
{
    auto buffer_stream = reinterpret_cast<mcl::ClientBufferStream*>(opaque_stream);
    if (!buffer_stream)
        return nullptr;

    return buffer_stream->set_scale(scale);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

void mir_buffer_stream_set_scale_sync(MirBufferStream* opaque_stream, float scale)
{
    auto wh = mir_buffer_stream_set_scale(opaque_stream, scale);
    if (wh)
        wh->wait_for_all();
}
