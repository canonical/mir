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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "mir_connection.h"
#include "render_surface.h"
#include "presentation_chain.h"
#include "mir/uncaught.h"
#include "mir/require.h"
#include "connection_surface_map.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
namespace
{
class RenderSurfaceResult
{
public:
    void set_result(MirRenderSurface* result)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        rs = result;
        cv.notify_all();
    }

    MirRenderSurface* wait_for_result()
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        cv.wait(lk, [this] { return rs; });
        return rs;
    }

private:
    MirRenderSurface* rs = nullptr;
    std::mutex mutex;
    std::condition_variable cv;
};

void set_result(MirRenderSurface* result, RenderSurfaceResult* context)
{
    context->set_result(result);
}

// 'Native window handle' (a.k.a. client-visible render surface) to connection map
class RenderSurfaceToConnectionMap
{
public:
    void insert(MirRenderSurface* render_surface_key, MirConnection* connection)
    {
        std::lock_guard<decltype(guard)> lk(guard);
        connections[render_surface_key] = connection;
    }

    void erase(void* render_surface_key)
    {
        std::lock_guard<decltype(guard)> lk(guard);
        auto conn_it = connections.find(render_surface_key);
        if (conn_it != connections.end())
            connections.erase(conn_it);
    }

    MirConnection* connection(void* render_surface_key) const
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

void mir_connection_create_render_surface(
    MirConnection* connection,
    int width, int height,
    MirRenderSurfaceCallback callback,
    void* context)
try
{
    mir::require(connection);
    if (auto rs = connection->create_render_surface_with_content({width, height}, callback, context))
    {
        connection_map.insert(rs, connection);
    }
    else
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Error creating native window"));
    }
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

MirRenderSurface* mir_connection_create_render_surface_sync(
    MirConnection* connection,
    int width, int height)
try
{
    RenderSurfaceResult result;
    mir_connection_create_render_surface(
        connection,
        width, height,
        reinterpret_cast<MirRenderSurfaceCallback>(set_result),
        &result);
    return result.wait_for_result();
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
    auto conn = connection_map.connection(render_surface);
    auto rs = conn->connection_surface_map()->render_surface(render_surface);
    mir::require(rs != nullptr);
    return rs->valid();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return false;
}

char const *mir_render_surface_get_error_message(
    MirRenderSurface* render_surface)
try
{
    mir::require(render_surface);
    auto conn = connection_map.connection(render_surface);
    auto rs = conn->connection_surface_map()->render_surface(render_surface);
    return rs->get_error_message();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return ex.what();
}

void mir_render_surface_release(
    MirRenderSurface* render_surface)
try
{
    mir::require(render_surface);
    auto connection = connection_map.connection(render_surface);
    connection_map.erase(render_surface);
    connection->release_render_surface_with_content(render_surface);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

MirBufferStream* mir_render_surface_get_buffer_stream(
    MirRenderSurface* render_surface,
    int width, int height,
    MirPixelFormat format)
try
{
    mir::require(render_surface);
    auto connection = connection_map.connection(render_surface);
    auto rs = connection->connection_surface_map()->render_surface(render_surface);
    return rs->get_buffer_stream(width, height, format, mir_buffer_usage_software);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

MirPresentationChain* mir_render_surface_get_presentation_chain(
    MirRenderSurface* render_surface)
try
{
    mir::require(render_surface);
    auto connection = connection_map.connection(render_surface);
    auto rs = connection->connection_surface_map()->render_surface(render_surface);
    return rs->get_presentation_chain();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

void mir_render_surface_get_size(MirRenderSurface* render_surface, int* width, int* height)
{
    mir::require(render_surface && width && height);
    auto connection = connection_map.connection(render_surface);
    auto rs = connection->connection_surface_map()->render_surface(render_surface);
    auto size = rs->size();
    *width = size.width.as_int();
    *height = size.height.as_int();
}

void mir_render_surface_set_size(MirRenderSurface* render_surface, int width, int height)
{
    mir::require(render_surface);
    auto connection = connection_map.connection(render_surface);
    auto rs = connection->connection_surface_map()->render_surface(render_surface);
    rs->set_size({width, height});
}

void mir_window_spec_set_cursor_render_surface(
    MirWindowSpec* spec,
    MirRenderSurface* surface,
    int hotspot_x, int hotspot_y)
{
    auto connection = connection_map.connection(surface);
    auto rs = connection->connection_surface_map()->render_surface(surface);
    spec->rendersurface_cursor = MirWindowSpec::RenderSurfaceCursor{rs->stream_id(), {hotspot_x, hotspot_y}};
}

MirCursorConfiguration* mir_cursor_configuration_from_render_surface(
    MirRenderSurface* surface,
    int hotspot_x, int hotspot_y)
try
{
    mir::require(surface);
    auto connection = connection_map.connection(surface);
    auto rs = connection->connection_surface_map()->render_surface(surface);
    return new MirCursorConfiguration(rs.get(), hotspot_x, hotspot_y);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

//temporary, until we stop trampolining via the RenderSurfaceToConnectionMap above
MirRenderSurface* mir::client::render_surface_lookup(void* key)
try
{
    auto connection = connection_map.connection(key);
    return connection->connection_surface_map()->render_surface(key).get();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}
#pragma GCC diagnostic pop
