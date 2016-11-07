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

namespace
{
void assign_result(void* result, void** context)
{
    if (context)
        *context = result;
}

// 'Native window handle' (a.k.a. client-visible render surface) to connection map
class RenderSurfaceToConnectionMap
{
public:
    void insert(void* const render_surface_key, MirConnection* const connection)
    {
        std::lock_guard<decltype(guard)> lk(guard);
        connections[render_surface_key] = connection;
    }

    void erase(void* const render_surface_key)
    {
        std::lock_guard<decltype(guard)> lk(guard);
        auto conn_it = connections.find(render_surface_key);
        if (conn_it != connections.end())
            connections.erase(conn_it);
    }

    MirConnection* connection(void* const render_surface_key) const
    {
        std::shared_lock<decltype(guard)> lk(guard);
        auto const it = connections.find(render_surface_key);
        if (it != connections.end())
            return it->second;
        else
            BOOST_THROW_EXCEPTION(std::runtime_error("could not find matching connection"));
    }
private:
    std::shared_timed_mutex mutable guard;
    std::unordered_map<void*, MirConnection*> connections;
};

RenderSurfaceToConnectionMap connection_map;
}

MirWaitHandle* mir_connection_create_render_surface_with_content(
    MirConnection* connection,
    int width, int height,
    mir_render_surface_callback callback,
    void* context)
try
{
    mir::require(connection);
    void* rs = nullptr;
    auto wh = connection->create_render_surface_with_content(mir::geometry::Size{width, height}, callback, context, &rs);
    if (!rs)
        BOOST_THROW_EXCEPTION(std::runtime_error("Error creating native window"));
    connection_map.insert(rs, connection);
    return wh;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

MirRenderSurface* mir_connection_create_render_surface_with_content_sync(
    MirConnection* connection,
    int width, int height)
try
{
    MirRenderSurface* render_surface = nullptr;
    auto wh = mir_connection_create_render_surface_with_content(
        connection,
        width, height,
        reinterpret_cast<mir_render_surface_callback>(assign_result),
        &render_surface);

    if (wh)
        wh->wait_for_all();

    return render_surface;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

bool mir_render_surface_with_content_is_valid(
        MirRenderSurface* render_surface)
try
{
    mir::require(render_surface);
    auto conn = connection_map.connection(static_cast<void*>(render_surface));
    auto rs = conn->connection_surface_map()->render_surface(render_surface);
    mir::require(rs != nullptr);
    return true;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return false;
}

void mir_render_surface_with_content_release(
        MirRenderSurface* render_surface)
try
{
    mir::require(render_surface);
/*    auto connection = connection_map.connection(static_cast<void*>(render_surface));
    auto rs = connection->connection_surface_map()->render_surface(render_surface);
    if (rs->stream_id().as_value() >= 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("Render surface still holds content"));

    connection_map.erase(static_cast<void*>(render_surface));
    connection->release_render_surface(render_surface);*/
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

MirBufferStream* mir_render_surface_with_content_get_buffer_stream(
    MirRenderSurface* render_surface,
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage usage)
try
{
    mir::require(render_surface);
    auto connection = connection_map.connection(static_cast<void*>(render_surface));
    auto rs = connection->connection_surface_map()->render_surface(render_surface);
    return rs->create_buffer_stream_from_id(width, height, format, usage);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

MirRenderSurface* mir_connection_create_render_surface(
    MirConnection* connection, int width, int height)
try
{
    mir::require(connection);
    auto rs = connection->create_render_surface({width, height});
    connection_map.insert(static_cast<void*>(rs), connection);
    return rs;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

bool mir_render_surface_is_valid(
    MirRenderSurface* render_surface)
try
{
    mir::require(render_surface);
    auto conn = connection_map.connection(static_cast<void*>(render_surface));
    auto rs = conn->connection_surface_map()->render_surface(render_surface);
    mir::require(rs != nullptr);
    return true;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return false;
}

void mir_render_surface_release(
    MirRenderSurface* render_surface)
try
{
    mir::require(render_surface);
    auto connection = connection_map.connection(static_cast<void*>(render_surface));
    auto rs = connection->connection_surface_map()->render_surface(render_surface);
    if (rs->stream_id().as_value() >= 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("Render surface still holds content"));

    connection_map.erase(static_cast<void*>(render_surface));
    connection->release_render_surface(render_surface);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

MirWaitHandle* mir_render_surface_create_buffer_stream(
        MirRenderSurface* render_surface,
        int width, int height,
        MirPixelFormat format,
        MirBufferUsage usage,
        mir_buffer_stream_callback callback,
        void* context)
try
{
    mir::require(render_surface);
    auto connection = connection_map.connection(static_cast<void*>(render_surface));
    auto rs = connection->connection_surface_map()->render_surface(render_surface);
    return rs->create_buffer_stream(
        width, height,
        format,
        usage,
        callback,
        context);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

MirBufferStream* mir_render_surface_create_buffer_stream_sync(
    MirRenderSurface* render_surface,
    int width, int height,
    MirPixelFormat format,
    MirBufferUsage usage)
try
{
    MirBufferStream* stream = nullptr;
    auto wh = mir_render_surface_create_buffer_stream(
        render_surface,
        width, height,
        format,
        usage,
        reinterpret_cast<mir_buffer_stream_callback>(assign_result),
        &stream);

    if (wh)
        wh->wait_for_all();

    return stream;
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

void mir_render_surface_get_size(MirRenderSurface* render_surface, int* width, int* height)
{
    auto connection = connection_map.connection(static_cast<void*>(render_surface));
    auto rs = connection->connection_surface_map()->render_surface(render_surface);
    auto size = rs->size();
    *width = size.width.as_int();
    *height = size.height.as_int();
}

void mir_render_surface_set_size(MirRenderSurface* render_surface, int width, int height)
{
    auto connection = connection_map.connection(static_cast<void*>(render_surface));
    auto rs = connection->connection_surface_map()->render_surface(render_surface);
    rs->set_size({width, height});
}
