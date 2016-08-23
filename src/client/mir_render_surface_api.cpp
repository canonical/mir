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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "mir_connection.h"
#include "render_surface.h"
#include "mir/uncaught.h"
#include "mir/require.h"
#include "connection_surface_map.h"

#if 0
namespace mcl = mir::client;

namespace
{
// assign_result is compatible with all 2-parameter callbacks
void assign_result(void* result, void** context)
{
    if (context)
        *context = result;
}
}
#endif

MirRenderSurface* mir_connection_create_render_surface(
    MirConnection* connection,
    int const width, int const height,
    MirPixelFormat const format)
try
{
    mir::require(connection);
    return connection->create_render_surface(width, height, format);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

bool mir_render_surface_is_valid(
    MirConnection* connection,
    MirRenderSurface* render_surface)
try
{
    mir::require(connection &&
                 render_surface &&
                 connection->connection_surface_map()->render_surface(render_surface));
    return true;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return false;
}

void mir_render_surface_release(
    MirConnection* connection,
    MirRenderSurface* render_surface)
try
{
    mir::require(connection && render_surface);
    connection->release_render_surface(render_surface);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

#if 0
MirBufferStream* mir_render_surface_create_buffer_stream_sync(
    MirRenderSurface* render_surface,
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage buffer_usage)
try
{
    mir::require(render_surface);
    MirBufferStream* stream = nullptr;
    auto rs = reinterpret_cast<mcl::RenderSurface*>(render_surface);

    auto wh = rs->create_client_buffer_stream(
        width,
        height,
        format,
        buffer_usage,
        reinterpret_cast<mir_buffer_stream_callback>(assign_result),
        &stream);
    wh->wait_for_all();
    return stream;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

MirConnection* mir_render_surface_connection(
    MirRenderSurface* render_surface)
try
{
    mir::require(render_surface);
    return render_surface->connection();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

MirSurface* mir_render_surface_container(
    MirRenderSurface* render_surface)
try
{
    mir::require(render_surface);

    return render_surface->container();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

MirEGLNativeWindowType mir_render_surface_egl_native_window(
    MirRenderSurface* render_surface)
try
{
    mir::require(render_surface);

    return render_surface->egl_native_window();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}
#endif
