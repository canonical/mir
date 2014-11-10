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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_connection_api.h"
#include "mir_toolkit/mir_connection.h"
#include "mir_toolkit/mir_client_library_drm.h"
#include "mir/default_configuration.h"
#include "mir/raii.h"

#include "mir_connection.h"
//#include "egl_native_display_container.h"
#include "default_connection_configuration.h"
#include "display_configuration.h"
#include "error_connections.h"


#include <unordered_set>
#include <cstddef>
#include <cstring>

namespace mcl = mir::client;

namespace
{
// assign_result is compatible with all 2-parameter callbacks
void assign_result(void* result, void** context)
{
    if (context)
        *context = result;
}

size_t division_ceiling(size_t a, size_t b)
{
    return ((a - 1) / b) + 1;
}

class DefaultMirConnectionAPI : public mcl::MirConnectionAPI
{
public:
    MirWaitHandle* connect(
        mcl::ConfigurationFactory configuration,
        char const* socket_file,
        char const* name,
        mir_connected_callback callback,
        void* context) override
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

            auto const conf = configuration(sock);

            std::unique_ptr<MirConnection> connection{new MirConnection(*conf)};
            auto const result = connection->connect(name, callback, context);
            connection.release();
            return result;
        }
        catch (std::exception const& x)
        {
            MirConnection* error_connection = new MirConnection(x.what());
            mcl::ErrorConnections::instance().insert(error_connection);
            callback(error_connection, context);
            return nullptr;
        }
    }

    void release(MirConnection* connection) override
    {
        if (!mcl::ErrorConnections::instance().contains(connection))
        {
            try
            {
                auto wait_handle = connection->disconnect();
                wait_handle->wait_for_all();
            }
            catch (std::exception const&)
            {
                // We're implementing a C API so no exceptions are to be
                // propagated. And that's OK because if disconnect() fails,
                // we don't care why. We're finished with the connection anyway.
            }
        }
        else
        {
            mcl::ErrorConnections::instance().remove(connection);
        }

        delete connection;
    }

    mcl::ConfigurationFactory configuration_factory() override
    {
        return [](std::string const& socket) {
            return std::unique_ptr<mcl::ConnectionConfiguration>{
                new mcl::DefaultConnectionConfiguration{socket}
            };
        };
    }
};

DefaultMirConnectionAPI default_api;
}

mcl::MirConnectionAPI* mir_connection_api_impl{&default_api};

MirWaitHandle* mir_connect(
    char const* socket_file,
    char const* name,
    mir_connected_callback callback,
    void* context)
{
    try
    {
        return mir_connection_api_impl->connect(mir_connection_api_impl->configuration_factory(),
                                                socket_file,
                                                name,
                                                callback,
                                                context);
    }
    catch (std::exception const&)
    {
        return nullptr;
    }
}

MirConnection* mir_connect_sync(
    char const* server,
    char const* app_name)
{
    MirConnection* conn = nullptr;
    mir_wait_for(mir_connect(server, app_name,
                             reinterpret_cast<mir_connected_callback>
                                             (assign_result),
                             &conn));
    return conn;
}

MirBool mir_connection_is_valid(MirConnection* connection)
{
    return MirConnection::is_valid(connection) ? mir_true : mir_false;
}

char const* mir_connection_get_error_message(MirConnection* connection)
{
    return connection->get_error_message();
}

void mir_connection_release(MirConnection* connection)
{
    try
    {
        return mir_connection_api_impl->release(connection);
    }
    catch (std::exception const&)
    {
    }
}

void mir_connection_get_platform(
    MirConnection* connection,
    MirPlatformPackage* platform_package)
{
    connection->populate(*platform_package);
}

void mir_connection_set_lifecycle_event_callback(
    MirConnection* connection,
    mir_lifecycle_event_callback callback,
    void* context)
{
    if (!mcl::ErrorConnections::instance().contains(connection))
        connection->register_lifecycle_event_callback(callback, context);
}

//TODO: DEPRECATED: remove this function
void mir_connection_get_display_info(
    MirConnection* connection,
    MirDisplayInfo* display_info)
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

MirDisplayConfiguration* mir_connection_create_display_config(
    MirConnection* connection)
{
    if (connection)
        return connection->create_copy_of_display_config();
    return nullptr;
}

void mir_connection_set_display_config_change_callback(
    MirConnection* connection,
    mir_display_config_callback callback,
    void* context)
{
    if (connection)
        connection->register_display_change_callback(callback, context);
}

void mir_display_config_destroy(MirDisplayConfiguration* configuration)
{
    mcl::delete_config_storage(configuration);
}

MirWaitHandle* mir_connection_apply_display_config(
    MirConnection* connection,
    MirDisplayConfiguration* display_configuration)
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

MirEGLNativeDisplayType mir_connection_get_egl_native_display(
    MirConnection* connection)
{
    return connection->egl_native_display();
}

void mir_connection_get_available_surface_formats(
    MirConnection* connection,
    MirPixelFormat* formats,
    unsigned const int format_size,
    unsigned int* num_valid_formats)
{
    if ((connection) && (formats) && (num_valid_formats))
        connection->available_surface_formats(formats, format_size, *num_valid_formats);
}

/**************************
 * DRM specific functions *
 **************************/

MirWaitHandle* mir_connection_drm_auth_magic(MirConnection* connection,
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
