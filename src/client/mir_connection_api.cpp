/*
 * Copyright © 2014,2015 Canonical Ltd.
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

#define MIR_LOG_COMPONENT "MirConnectionAPI"

#include "mir_connection_api.h"
#include "mir_toolkit/mir_connection.h"
#include "mir/default_configuration.h"
#include "mir/raii.h"

#include "mir_connection.h"
#include "default_connection_configuration.h"
#include "display_configuration.h"
#include "error_connections.h"
#include "mir/uncaught.h"

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

            auto connection = std::make_unique<MirConnection>(*conf);
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
            catch (std::exception const& ex)
            {
                // We're implementing a C API so no exceptions are to be
                // propagated. And that's OK because if disconnect() fails,
                // we don't care why. We're finished with the connection anyway.

                MIR_LOG_UNCAUGHT_EXCEPTION(ex);
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
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
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

bool mir_connection_is_valid(MirConnection* connection)
{
    return MirConnection::is_valid(connection);
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
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
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
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
        return nullptr;
    }
}

MirEGLNativeDisplayType mir_connection_get_egl_native_display(
    MirConnection* connection)
{
    return connection->egl_native_display();
}

MirPixelFormat mir_connection_get_egl_pixel_format(MirConnection* connection,
    EGLDisplay disp, EGLConfig conf)
{
    return connection->egl_pixel_format(disp, conf);
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

MirWaitHandle* mir_connection_platform_operation(
    MirConnection* connection,
    MirPlatformMessage const* request,
    mir_platform_operation_callback callback, void* context)
{
    try
    {
        return connection->platform_operation(request, callback, context);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
        return nullptr;
    }
}
