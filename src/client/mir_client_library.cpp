/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir/default_configuration.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_client_library_drm.h"
#include "mir_toolkit/mir_client_library_lightdm.h"

#include "mir_connection.h"
#include "mir_surface.h"
#include "native_client_platform_factory.h"
#include "egl_native_display_container.h"
#include "mir_logger.h"
#include "make_rpc_channel.h"

#include <set>
#include <unordered_set>
#include <cstddef>

namespace mcl = mir::client;

std::mutex MirConnection::connection_guard;
std::unordered_set<MirConnection*> MirConnection::valid_connections;

namespace
{
MirConnection error_connection;

// assign_result is compatible with all 2-parameter callbacks
void assign_result(void *result, void **context)
{
    if (context)
        *context = result;
}

}

MirWaitHandle* mir_connect(char const* socket_file, char const* name, mir_connected_callback callback, void * context)
{

    try
    {
        const std::string sock = socket_file ? socket_file :
                                               mir::default_server_socket;
        auto log = std::make_shared<mcl::ConsoleLogger>();
        auto client_platform_factory = std::make_shared<mcl::NativeClientPlatformFactory>();

        MirConnection* connection = new MirConnection(
            mcl::make_rpc_channel(sock, log),
            log,
            client_platform_factory);

        return connection->connect(name, callback, context);
    }
    catch (std::exception const& x)
    {
        error_connection.set_error_message(x.what());
        callback(&error_connection, context);
        return 0;
    }
}

MirConnection *mir_connect_sync(char const *server,
                                             char const *app_name)
{
    MirConnection *conn = nullptr;
    mir_wait_for(mir_connect(server, app_name,
                             reinterpret_cast<mir_connected_callback>
                                             (assign_result),
                             &conn));
    return conn;
}

int mir_connection_is_valid(MirConnection * connection)
{
    return MirConnection::is_valid(connection);
}

char const * mir_connection_get_error_message(MirConnection * connection)
{
    return connection->get_error_message();
}

void mir_connection_release(MirConnection * connection)
{
    if (&error_connection == connection) return;

    auto wait_handle = connection->disconnect();
    wait_handle->wait_for_result();

    delete connection;
}

MirEGLNativeDisplayType mir_connection_get_egl_native_display(MirConnection *connection)
{
    return connection->egl_native_display();
}

MirWaitHandle* mir_connection_create_surface(
    MirConnection* connection,
    MirSurfaceParameters const* params,
    mir_surface_lifecycle_callback callback,
    void* context)
{
    if (&error_connection == connection) return 0;

    try
    {
        return connection->create_surface(*params, callback, context);
    }
    catch (std::exception const&)
    {
        // TODO callback with an error surface
        return 0; // TODO
    }

}

MirSurface* mir_connection_create_surface_sync(
    MirConnection* connection, 
    MirSurfaceParameters const* params)
{
    MirSurface *surface = nullptr;

    mir_wait_for(mir_connection_create_surface(connection, params,
        reinterpret_cast<mir_surface_lifecycle_callback>(assign_result),
        &surface));

    return surface;
}

void mir_surface_set_event_handler(MirSurface *surface,
                                   MirEventDelegate const *event_handler)
{
    surface->set_event_handler(event_handler);
}

MirWaitHandle* mir_surface_release(
    MirSurface * surface,
    mir_surface_lifecycle_callback callback, void * context)
{
    return surface->release_surface(callback, context);
}

void mir_surface_release_sync(MirSurface *surface)
{
    mir_wait_for(mir_surface_release(surface,
        reinterpret_cast<mir_surface_lifecycle_callback>(assign_result),
        nullptr));
}

int mir_surface_get_id(MirSurface * surface)
{
    return surface->id();
}

int mir_surface_is_valid(MirSurface* surface)
{
    return surface->is_valid();
}

char const * mir_surface_get_error_message(MirSurface * surface)
{
    return surface->get_error_message();
}

void mir_surface_get_parameters(MirSurface * surface, MirSurfaceParameters *parameters)
{
    *parameters = surface->get_parameters();
}

void mir_surface_get_current_buffer(MirSurface *surface, MirBufferPackage * buffer_package_out)
{
    auto package = surface->get_current_buffer_package();

    buffer_package_out->data_items = package->data_items;
    buffer_package_out->fd_items = package->fd_items;
    for(auto i=0; i<mir_buffer_package_max; i++)
    {
        buffer_package_out->data[i] = package->data[i];
        buffer_package_out->fd[i] = package->fd[i];
    }

    buffer_package_out->stride = package->stride;
}

void mir_connection_get_platform(MirConnection *connection, MirPlatformPackage *platform_package)
{
    connection->populate(*platform_package);
}

void mir_connection_get_display_info(MirConnection *connection, MirDisplayInfo *display_info)
{
    connection->populate(*display_info);
}

void mir_surface_get_graphics_region(MirSurface * surface, MirGraphicsRegion * graphics_region)
{
    surface->get_cpu_region( *graphics_region);
}

MirWaitHandle* mir_surface_next_buffer(MirSurface *surface, mir_surface_lifecycle_callback callback, void * context)
{
    return surface->next_buffer(callback, context);
}

void mir_surface_next_buffer_sync(MirSurface *surface)
{
    mir_wait_for(mir_surface_next_buffer(surface,
        reinterpret_cast<mir_surface_lifecycle_callback>(assign_result),
        nullptr));
}

void mir_wait_for(MirWaitHandle* wait_handle)
{
    if (wait_handle)
        wait_handle->wait_for_result();
}

MirEGLNativeWindowType mir_surface_get_egl_native_window(MirSurface *surface)
{
    return surface->generate_native_window();
}

MirWaitHandle *mir_connection_drm_auth_magic(MirConnection* connection,
                                             unsigned int magic,
                                             mir_drm_auth_magic_callback callback,
                                             void* context)
{
    return connection->drm_auth_magic(magic, callback, context);
}

MirWaitHandle *mir_connect_with_lightdm_id(
    char const *server,
    int lightdm_id,
    char const *app_name,
    mir_connected_callback callback,
    void *client_context)
try
{
    auto log = std::make_shared<mcl::ConsoleLogger>();
    auto client_platform_factory = std::make_shared<mcl::NativeClientPlatformFactory>();

    MirConnection* connection = new MirConnection(
        mcl::make_rpc_channel(server, log),
        log,
        client_platform_factory);

    return connection->connect(lightdm_id, app_name, callback, client_context);
}
catch (std::exception const& x)
{
    error_connection.set_error_message(x.what());
    callback(&error_connection, client_context);
    return 0;
}

void mir_select_focus_by_lightdm_id(MirConnection* connection, int lightdm_id)
try
{
    connection->select_focus_by_lightdm_id(lightdm_id);
}
catch (std::exception const&)
{
    // Ignore
}

MirWaitHandle* mir_surface_set_type(MirSurface *surf,
                                                           MirSurfaceType type)
{
    return surf ? surf->configure(mir_surface_attrib_type, type) : NULL;
}

MirSurfaceType mir_surface_get_type(MirSurface *surf)
{
    MirSurfaceType type = mir_surface_type_normal;

    if (surf)
    {
        // Only the client will ever change the type of a surface so it is
        // safe to get the type from a local cache surf->attrib().

        int t = surf->attrib(mir_surface_attrib_type);
        type = static_cast<MirSurfaceType>(t);
    }

    return type;
}
