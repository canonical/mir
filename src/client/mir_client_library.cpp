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
#include "mir/raii.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_client_library_drm.h"
#include "mir_toolkit/mir_client_library_debug.h"

#include "mir_connection.h"
#include "display_configuration.h"
#include "mir_surface.h"
#include "native_client_platform_factory.h"
#include "egl_native_display_container.h"
#include "default_connection_configuration.h"
#include "lifecycle_control.h"

#include <set>
#include <unordered_set>
#include <cstddef>
#include <cstring>

namespace mcl = mir::client;

namespace
{
class ConnectionList
{
public:
    void insert(MirConnection* connection)
    {
        std::lock_guard<std::mutex> lock(connection_guard);
        connections.insert(connection);
    }

    void remove(MirConnection* connection)
    {
        std::lock_guard<std::mutex> lock(connection_guard);
        connections.erase(connection);
    }

    bool contains(MirConnection* connection)
    {
        std::lock_guard<std::mutex> lock(connection_guard);
        return connections.count(connection);
    }

private:
    std::mutex connection_guard;
    std::unordered_set<MirConnection*> connections;
};

ConnectionList error_connections;

// assign_result is compatible with all 2-parameter callbacks
void assign_result(void *result, void **context)
{
    if (context)
        *context = result;
}

size_t division_ceiling(size_t a, size_t b)
{
    return ((a - 1) / b) + 1;
}

}

MirWaitHandle* mir_default_connect(
    char const* socket_file, char const* name, mir_connected_callback callback, void * context)
{

    try
    {
        std::string sock;
        if (socket_file)
            sock = socket_file;
        else
        {
            auto socket_env = getenv("MIR_SOCKET");
            if (socket_env)
                sock = socket_env;
            else
                sock = mir::default_server_socket;
        }

        mcl::DefaultConnectionConfiguration conf{sock};

        std::unique_ptr<MirConnection> connection{new MirConnection(conf)};
        auto const result = connection->connect(name, callback, context);
        connection.release();
        return result;
    }
    catch (std::exception const& x)
    {
        MirConnection* error_connection = new MirConnection(x.what());
        error_connections.insert(error_connection);
        callback(error_connection, context);
        return nullptr;
    }
}


void mir_default_connection_release(MirConnection * connection)
{
    if (!error_connections.contains(connection))
    {
        auto wait_handle = connection->disconnect();
        wait_handle->wait_for_all();
    }
    else
    {
        error_connections.remove(connection);
    }

    delete connection;
}

//mir_connect and mir_connection_release can be overridden by test code that sets these function
//pointers to do things like stub out the graphics drivers or change the connection configuration.

//TODO: we could have a more comprehensive solution that allows us to substitute any of the functions
//for test purposes, not just the connect functions
MirWaitHandle* (*mir_connect_impl)(
    char const *server, char const *app_name,
    mir_connected_callback callback, void *context) = mir_default_connect;
void (*mir_connection_release_impl) (MirConnection *connection) = mir_default_connection_release;

MirWaitHandle* mir_connect(char const* socket_file, char const* name, mir_connected_callback callback, void * context)
{
    try
    {
        return mir_connect_impl(socket_file, name, callback, context);
    }
    catch (std::exception const&)
    {
        return nullptr;
    }
}

void mir_connection_release(MirConnection *connection)
{
    try
    {
        return mir_connection_release_impl(connection);
    }
    catch (std::exception const&)
    {
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

MirBool mir_connection_is_valid(MirConnection * connection)
{
    return MirConnection::is_valid(connection) ? mir_true : mir_false;
}

char const * mir_connection_get_error_message(MirConnection * connection)
{
    return connection->get_error_message();
}

MirEGLNativeDisplayType mir_connection_get_egl_native_display(MirConnection *connection)
{
    return connection->egl_native_display();
}

void mir_connection_get_available_surface_formats(
    MirConnection * connection, MirPixelFormat* formats,
    unsigned const int format_size, unsigned int *num_valid_formats)
{
    if ((connection) && (formats) && (num_valid_formats))
        connection->available_surface_formats(formats, format_size, *num_valid_formats);
}

MirWaitHandle* mir_connection_create_surface(
    MirConnection* connection,
    MirSurfaceParameters const* params,
    mir_surface_callback callback,
    void* context)
{
    if (error_connections.contains(connection)) return 0;

    try
    {
        return connection->create_surface(*params, callback, context);
    }
    catch (std::exception const&)
    {
        // TODO callback with an error surface
        return nullptr;
    }

}

MirSurface* mir_connection_create_surface_sync(
    MirConnection* connection,
    MirSurfaceParameters const* params)
{
    MirSurface *surface = nullptr;

    mir_wait_for(mir_connection_create_surface(connection, params,
        reinterpret_cast<mir_surface_callback>(assign_result),
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
    mir_surface_callback callback, void * context)
{
    try
    {
        return surface->release_surface(callback, context);
    }
    catch (std::exception const&)
    {
        return nullptr;
    }
}

void mir_surface_release_sync(MirSurface *surface)
{
    mir_wait_for(mir_surface_release(surface,
        reinterpret_cast<mir_surface_callback>(assign_result),
        nullptr));
}

int mir_surface_get_id(MirSurface * surface)
{
    return mir_debug_surface_id(surface);
}

int mir_debug_surface_id(MirSurface * surface)
{
    return surface->id();
}

MirBool mir_surface_is_valid(MirSurface* surface)
{
    return surface->is_valid() ? mir_true : mir_false;
}

char const * mir_surface_get_error_message(MirSurface * surface)
{
    return surface->get_error_message();
}

void mir_surface_get_parameters(MirSurface * surface, MirSurfaceParameters *parameters)
{
    *parameters = surface->get_parameters();
}

MirPlatformType mir_surface_get_platform_type(MirSurface * surface)
{
    return surface->platform_type();
}

void mir_surface_get_current_buffer(MirSurface * surface, MirNativeBuffer ** buffer_package_out)
{
    *buffer_package_out = surface->get_current_buffer_package();
}

uint32_t mir_debug_surface_current_buffer_id(MirSurface * surface)
{
    return surface->get_current_buffer_id();
}

void mir_connection_get_platform(MirConnection *connection, MirPlatformPackage *platform_package)
{
    connection->populate(*platform_package);
}

MirDisplayConfiguration* mir_connection_create_display_config(MirConnection *connection)
{
    if (connection)
        return connection->create_copy_of_display_config();
    return nullptr;
}

void mir_display_config_destroy(MirDisplayConfiguration* configuration)
{
    mcl::delete_config_storage(configuration);
}

//TODO: DEPRECATED: remove this function
void mir_connection_get_display_info(MirConnection *connection, MirDisplayInfo *display_info)
{
    auto const config = mir::raii::deleter_for(
        mir_connection_create_display_config(connection),
        &mir_display_config_destroy);

    if (config->num_outputs < 1)
        return;

    MirDisplayOutput* state = nullptr;
    // We can't handle more than one display, so just populate based on the first
    // active display we find.
    for (unsigned int i = 0; i < config->num_outputs; ++i)
    {
        if (config->outputs[i].used && config->outputs[i].connected &&
            config->outputs[i].current_mode < config->outputs[i].num_modes)
        {
            state = &config->outputs[i];
            break;
        }
    }
    // Oh, oh! No connected outputs?!
    if (state == nullptr)
    {
        memset(display_info, 0, sizeof(*display_info));
        return;
    }

    MirDisplayMode mode = state->modes[state->current_mode];

    display_info->width = mode.horizontal_resolution;
    display_info->height = mode.vertical_resolution;

    unsigned int format_items;
    if (state->num_output_formats > mir_supported_pixel_format_max)
         format_items = mir_supported_pixel_format_max;
    else
         format_items = state->num_output_formats;

    display_info->supported_pixel_format_items = format_items;
    for(auto i=0u; i < format_items; i++)
    {
        display_info->supported_pixel_format[i] = state->output_formats[i];
    }
}

void mir_surface_get_graphics_region(MirSurface * surface, MirGraphicsRegion * graphics_region)
{
    surface->get_cpu_region( *graphics_region);
}

MirWaitHandle* mir_surface_swap_buffers(MirSurface *surface, mir_surface_callback callback, void * context)
try
{
    return surface->next_buffer(callback, context);
}
catch (std::exception const&)
{
    return nullptr;
}

void mir_surface_swap_buffers_sync(MirSurface *surface)
{
    mir_wait_for(mir_surface_swap_buffers(surface,
        reinterpret_cast<mir_surface_callback>(assign_result),
        nullptr));
}

void mir_wait_for(MirWaitHandle* wait_handle)
{
    if (wait_handle)
        wait_handle->wait_for_all();
}

void mir_wait_for_one(MirWaitHandle* wait_handle)
{
    if (wait_handle)
        wait_handle->wait_for_one();
}

MirEGLNativeWindowType mir_surface_get_egl_native_window(MirSurface *surface)
{
    return surface->generate_native_window();
}

MirWaitHandle* mir_surface_set_type(MirSurface *surf,
                                    MirSurfaceType type)
{
    try
    {
        return surf ? surf->configure(mir_surface_attrib_type, type) : nullptr;
    }
    catch (std::exception const&)
    {
        return nullptr;
    }
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

MirWaitHandle* mir_surface_set_state(MirSurface *surf, MirSurfaceState state)
{
    try
    {
        return surf ? surf->configure(mir_surface_attrib_state, state) : nullptr;
    }
    catch (std::exception const&)
    {
        return nullptr;
    }
}

MirSurfaceState mir_surface_get_state(MirSurface *surf)
{
    MirSurfaceState state = mir_surface_state_unknown;

    try
    {
        if (surf)
        {
            int s = surf->attrib(mir_surface_attrib_state);

            if (s == mir_surface_state_unknown)
            {
                surf->configure(mir_surface_attrib_state,
                                mir_surface_state_unknown)->wait_for_all();
                s = surf->attrib(mir_surface_attrib_state);
            }

            state = static_cast<MirSurfaceState>(s);
        }
    }
    catch (std::exception const&)
    {
    }

    return state;
}

MirWaitHandle* mir_surface_set_swapinterval(MirSurface* surf, int interval)
{
    if ((interval < 0) || (interval > 1))
        return nullptr;

    try
    {
        return surf ? surf->configure(mir_surface_attrib_swapinterval, interval) : nullptr;
    }
    catch (std::exception const&)
    {
        return nullptr;
    }
}

int mir_surface_get_swapinterval(MirSurface* surf)
{
    return surf ? surf->attrib(mir_surface_attrib_swapinterval) : -1;
}

void mir_connection_set_lifecycle_event_callback(MirConnection* connection,
    mir_lifecycle_event_callback callback, void* context)
{
    if (!error_connections.contains(connection))
        connection->register_lifecycle_event_callback(callback, context);
}

void mir_connection_set_display_config_change_callback(MirConnection* connection,
    mir_display_config_callback callback, void* context)
{
    if (connection)
        connection->register_display_change_callback(callback, context);
}

MirWaitHandle* mir_connection_apply_display_config(MirConnection *connection, MirDisplayConfiguration* display_configuration)
{
    try
    {
        return connection ? connection->configure_display(display_configuration) : nullptr;
    }
    catch (std::exception const&)
    {
        return nullptr;
    }
}

/**************************
 * DRM specific functions *
 **************************/

MirWaitHandle *mir_connection_drm_auth_magic(MirConnection* connection,
                                             unsigned int magic,
                                             mir_drm_auth_magic_callback callback,
                                             void* context)
{
    return connection->drm_auth_magic(magic, callback, context);
}

int mir_connection_drm_set_gbm_device(MirConnection* connection,
                                      struct gbm_device* gbm_dev)
{
    size_t const pointer_size_in_ints = division_ceiling(sizeof(gbm_dev), sizeof(int));
    std::vector<int> extra_data(pointer_size_in_ints);

    memcpy(extra_data.data(), &gbm_dev, sizeof(gbm_dev));

    return connection->set_extra_platform_data(extra_data);
}
