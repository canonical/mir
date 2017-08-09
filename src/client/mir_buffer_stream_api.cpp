/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#define MIR_LOG_COMPONENT "MirBufferStreamAPI"

#include "mir_screencast.h"
#include "mir_surface.h"
#include "mir_connection.h"
#include "buffer_stream.h"
#include "render_surface.h"

#include "mir_toolkit/mir_buffer.h"
#include "mir/client/client_buffer.h"

#include "mir/uncaught.h"
#include "mir/require.h"

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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
MirWaitHandle* mir_connection_create_buffer_stream(MirConnection *connection,
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage buffer_usage,
    MirBufferStreamCallback callback,
    void *context)
try
{
    return connection->create_client_buffer_stream(
        width, height, format, buffer_usage, nullptr, callback, context);
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
    MirBufferStream *stream = nullptr;
    mir_connection_create_buffer_stream(connection, width, height, format, buffer_usage,
        reinterpret_cast<MirBufferStreamCallback>(assign_result), &stream)->wait_for_all();
    return stream;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}
#pragma GCC diagnostic pop

MirWaitHandle* mir_buffer_stream_release(
    MirBufferStream* buffer_stream,
    MirBufferStreamCallback callback,
    void* context)
{
    auto connection = buffer_stream->connection();
    return connection->release_buffer_stream(buffer_stream, callback, context);
}

void mir_buffer_stream_release_sync(MirBufferStream *buffer_stream)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_buffer_stream_release(buffer_stream, nullptr, nullptr)->wait_for_all();
#pragma GCC diagnostic pop
}

void mir_buffer_stream_get_current_buffer(MirBufferStream* buffer_stream, MirNativeBuffer** buffer_package_out)
try
{
    *buffer_package_out = buffer_stream->get_current_buffer_package();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

MirWaitHandle* mir_buffer_stream_swap_buffers(
    MirBufferStream* buffer_stream,
    MirBufferStreamCallback callback,
    void* context)
try
{
    /*
     * TODO: Add client-side vsync support for mir_buffer_stream_swap_buffers()
     *       Not in a hurry though, because the old server-side vsync is still
     *       present and AFAIK the only user of swap_buffers callbacks is Xmir.
     *       There are many ways to approach the problem and some more
     *       contentious than others, so do it later.
     */
    return buffer_stream->swap_buffers([buffer_stream, callback, context]{
            if (callback)
                callback(buffer_stream, context);
    });
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

void mir_buffer_stream_swap_buffers_sync(MirBufferStream* buffer_stream)
try
{
    buffer_stream->swap_buffers_sync();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

bool mir_buffer_stream_get_graphics_region(
    MirBufferStream *buffer_stream,
    MirGraphicsRegion *region_out)
try
{
    auto secured_region = buffer_stream->secure_for_cpu_write();
    region_out->width = secured_region->width.as_uint32_t();
    region_out->height = secured_region->height.as_uint32_t();
    region_out->stride = secured_region->stride.as_uint32_t();
    region_out->pixel_format = secured_region->format;
    region_out->vaddr = secured_region->vaddr.get();
    return true;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return false;
}

MirEGLNativeWindowType mir_buffer_stream_get_egl_native_window(MirBufferStream* buffer_stream)
try
{
    return reinterpret_cast<MirEGLNativeWindowType>(buffer_stream->egl_native_window());
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return MirEGLNativeWindowType();
}

MirPlatformType mir_buffer_stream_get_platform_type(MirBufferStream* buffer_stream)
try
{
    return buffer_stream->platform_type();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return MirPlatformType();
}

bool mir_buffer_stream_is_valid(MirBufferStream* opaque_stream)
{
    return opaque_stream->valid();
}

MirWaitHandle* mir_buffer_stream_set_scale(MirBufferStream* buffer_stream, float scale)
try
{
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto wh = mir_buffer_stream_set_scale(opaque_stream, scale);
#pragma GCC diagnostic pop
    if (wh)
        wh->wait_for_all();
}

char const* mir_buffer_stream_get_error_message(MirBufferStream* buffer_stream)
{
    return buffer_stream->get_error_message();
}

MirWaitHandle* mir_buffer_stream_set_swapinterval(MirBufferStream* buffer_stream, int interval)
try
{
    if (interval < 0)
        return nullptr;

    if (!buffer_stream)
        return nullptr;

    return buffer_stream->set_swap_interval(interval);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

int mir_buffer_stream_get_swapinterval(MirBufferStream* buffer_stream)
try
{
    if (buffer_stream)
        return buffer_stream->swap_interval();
    else
        return -1;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return -1;
}

unsigned long mir_buffer_stream_get_microseconds_till_vblank(
    MirBufferStream const* stream)
{
    mir::require(stream);
    return stream->microseconds_till_vblank().count();
}

void mir_buffer_stream_set_size(MirBufferStream* stream, int width, int height)
try
{
    mir::require(stream);
    return stream->set_size(mir::geometry::Size{width, height});
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_buffer_stream_get_size(MirBufferStream* stream, int* width, int* height)
try
{
    mir::require(stream);
    mir::require(width);
    mir::require(height);
    auto size = stream->size();
    *width = size.width.as_int();
    *height = size.height.as_int();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    *width = -1;
    *height = -1;
}
