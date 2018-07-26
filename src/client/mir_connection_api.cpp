/*
 * Copyright © 2014, 2015 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#define MIR_LOG_COMPONENT "MirConnectionAPI"

#include "mir_toolkit/mir_connection.h"
#include "mir/default_configuration.h"
#include "mir/input/input_devices.h"
#include "mir/raii.h"
#include "mir/require.h"

#include "mir_connection.h"
#include "default_connection_configuration.h"
#include "display_configuration.h"
#include "error_connections.h"
#include "mir/uncaught.h"

#include <cstddef>
#include <cstring>

namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace mi = mir::input;

namespace
{
// assign_result is compatible with all 2-parameter callbacks
void assign_result(void* result, void** context)
{
    if (context)
        *context = result;
}
}

MirWaitHandle* mir_connect(
    char const* socket_file,
    char const* name,
    MirConnectedCallback callback,
    void* context)
try
{
    try
    {
        std::string sock;
        if (socket_file)
            sock = socket_file;
        else
        {
            auto socket_env = getenv("MIR_SOCKET");
            if (socket_env && socket_env[0])
                sock = socket_env;
            else
                sock = mir::default_server_socket;
        }

        mcl::DefaultConnectionConfiguration conf(sock);

        auto connection = std::make_unique<MirConnection>(conf);
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
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

MirConnection* mir_connect_sync(
    char const* server,
    char const* app_name)
{
    MirConnection* conn = nullptr;
    auto wh = mir_connect(server, app_name,
                          reinterpret_cast<MirConnectedCallback>
                          (assign_result),
                          &conn);

    if (wh != nullptr)
    {
        wh->wait_for_all();
    }

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
try
{
    std::unique_ptr<MirConnection> scoped_deleter{connection};
    if (!mcl::ErrorConnections::instance().contains(connection))
    {
        auto wait_handle = connection->disconnect();
        wait_handle->wait_for_all();
    }
    else
    {
        mcl::ErrorConnections::instance().remove(connection);
    }
}
catch (std::exception const& ex)
{
    // We're implementing a C API so no exceptions are to be
    // propagated. And that's OK because if disconnect() fails,
    // we don't care why. We're finished with the connection anyway.
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_connection_get_platform(
    MirConnection* connection,
    MirPlatformPackage* platform_package)
{
    connection->populate(*platform_package);
}

void mir_connection_get_graphics_module(MirConnection *connection, MirModuleProperties *properties)
try
{
    mir::require(mir_connection_is_valid(connection));
    mir::require(properties != nullptr);

    connection->populate_graphics_module(*properties);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_connection_set_lifecycle_event_callback(
    MirConnection* connection,
    MirLifecycleEventCallback callback,
    void* context)
{
    if (!mcl::ErrorConnections::instance().contains(connection))
        connection->register_lifecycle_event_callback(callback, context);
}

void mir_connection_set_ping_event_callback(
    MirConnection* connection,
    MirPingEventCallback callback,
    void* context)
{
    if (!mcl::ErrorConnections::instance().contains(connection))
        connection->register_ping_event_callback(callback, context);
}

void mir_connection_pong(MirConnection *connection, int32_t serial)
{
    if (!mcl::ErrorConnections::instance().contains(connection))
        connection->pong(serial);
}

MirDisplayConfiguration* mir_connection_create_display_config(
    MirConnection* connection)
{
    if (connection)
        return connection->create_copy_of_display_config();
    return nullptr;
}

MirDisplayConfig* mir_connection_create_display_configuration(
    MirConnection* connection)
{
    mir::require(mir_connection_is_valid(connection));

    return new MirDisplayConfig(*connection->snapshot_display_configuration());
}

void mir_display_config_release(MirDisplayConfig* config)
{
    delete config;
}

void mir_connection_set_display_config_change_callback(
    MirConnection* connection,
    MirDisplayConfigCallback callback,
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

MirWaitHandle* mir_connection_set_base_display_config(
    MirConnection* connection,
    MirDisplayConfiguration const* display_configuration)
{
    try
    {
        return connection ? connection->set_base_display_configuration(display_configuration) : nullptr;
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
        return nullptr;
    }
}

MirInputConfig* mir_connection_create_input_config(
    MirConnection* connection)
try
{
    mir::require(mir_connection_is_valid(connection));

    auto wrapper = connection->the_input_devices();
    return new MirInputConfig{wrapper->devices()};
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    return nullptr;
}

void mir_connection_set_input_config_change_callback(
    MirConnection* connection,
    MirInputConfigCallback callback,
    void* context)
try
{
    if (!connection)
        return;
    auto devices = connection->the_input_devices();
    devices->set_change_callback([connection, context, callback]{callback(connection, context);});
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_connection_apply_session_input_config(MirConnection* connection, MirInputConfig const* config) try
{
    if (!connection)
        return;
    connection->apply_input_configuration(config);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_connection_set_base_input_config(MirConnection* connection, MirInputConfig const* config) try
{
    if (!connection)
        return;
    connection->set_base_input_configuration(config);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_input_config_destroy(MirInputConfig const* config)
{
    mir_input_config_release(config);
}

void mir_input_config_release(MirInputConfig const* config)
{
    delete config;
}

void mir_connection_preview_base_display_configuration(
    MirConnection* connection,
    MirDisplayConfig const* config,
    int timeout_seconds)
{
    mir::require(mir_connection_is_valid(connection));

    try
    {
        connection->preview_base_display_configuration(*config, std::chrono::seconds{timeout_seconds});
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }
}

void mir_connection_confirm_base_display_configuration(
    MirConnection* connection,
    MirDisplayConfig const* config)
{
    mir::require(mir_connection_is_valid(connection));

    try
    {
        connection->confirm_base_display_configuration(*config);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }
}

void mir_connection_cancel_base_display_configuration_preview(
    MirConnection* connection)
{
    mir::require(mir_connection_is_valid(connection));

    try
    {
        connection->cancel_base_display_configuration_preview();
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
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
    MirPlatformOperationCallback callback, void* context)
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

void mir_connection_set_error_callback(
    MirConnection* connection,
    MirErrorCallback callback,
    void* context)
{
    mir::require(mir_connection_is_valid(connection));

    try
    {
        connection->register_error_callback(callback, context);
    }
    catch (std::exception const& ex)
    {
        MIR_LOG_UNCAUGHT_EXCEPTION(ex);
    }
}

unsigned mir_get_client_api_version()
{
    return MIR_CLIENT_API_VERSION;
}

void mir_connection_apply_session_display_config(MirConnection* connection, MirDisplayConfig const* display_config)
try
{
    connection->configure_session_display(*display_config);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_connection_remove_session_display_config(MirConnection* connection)
try
{
    connection->remove_session_display();
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

void mir_connection_enumerate_extensions(
    MirConnection* connection,
    void* context,
    void (*enumerator)(void* context, char const* extension, int version))
try
{
    mir::require(mir_connection_is_valid(connection));
    connection->enumerate_extensions(context, enumerator);
}
catch (std::exception const& ex)
{
    MIR_LOG_UNCAUGHT_EXCEPTION(ex);
}

