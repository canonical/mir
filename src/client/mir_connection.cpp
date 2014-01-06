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

#include "mir/logging/logger.h"

#include "mir_connection.h"
#include "mir_surface.h"
#include "client_platform.h"
#include "client_platform_factory.h"
#include "rpc/mir_basic_rpc_channel.h"
#include "connection_configuration.h"
#include "display_configuration.h"
#include "connection_surface_map.h"
#include "lifecycle_control.h"

#include <algorithm>
#include <cstddef>
#include <unistd.h>
#include <signal.h>

namespace mcl = mir::client;
namespace mircv = mir::input::receiver;
namespace gp = google::protobuf;

namespace
{

class MirDisplayConfigurationStore
{
public:
    MirDisplayConfigurationStore(MirDisplayConfiguration* config)
        : config{config}
    {
    }

    ~MirDisplayConfigurationStore()
    {
        mcl::delete_config_storage(config);
    }

    MirDisplayConfiguration* operator->() const { return config; }

private:
    MirDisplayConfigurationStore(MirDisplayConfigurationStore const&) = delete;
    MirDisplayConfigurationStore& operator=(MirDisplayConfigurationStore const&) = delete;

    MirDisplayConfiguration* const config;
};

std::mutex connection_guard;
std::unordered_set<MirConnection*> valid_connections;
}

MirConnection::MirConnection(std::string const& error_message) :
    channel(),
    server(0),
    error_message(error_message)
{
}

MirConnection::MirConnection(
    mir::client::ConnectionConfiguration& conf) :
        channel(conf.the_rpc_channel()),
        server(channel.get(), ::google::protobuf::Service::STUB_DOESNT_OWN_CHANNEL),
        logger(conf.the_logger()),
        client_platform_factory(conf.the_client_platform_factory()),
        input_platform(conf.the_input_platform()),
        display_configuration(conf.the_display_configuration()),
        lifecycle_control(conf.the_lifecycle_control()),
        surface_map(conf.the_surface_map())
{
    connect_result.set_error("connect not called");
    {
        std::lock_guard<std::mutex> lock(connection_guard);
        valid_connections.insert(this);
    }
}

MirConnection::~MirConnection() noexcept
{
    // We don't die while if are pending callbacks (as they touch this).
    // But, if after 500ms we don't get a call, assume it won't happen.
    connect_wait_handle.wait_for_pending(std::chrono::milliseconds(500));

    {
        std::lock_guard<std::mutex> lock(connection_guard);
        valid_connections.erase(this);
    }

    std::lock_guard<decltype(mutex)> lock(mutex);
    if (connect_result.has_platform())
    {
        auto const& platform = connect_result.platform();
        for (int i = 0, end = platform.fd_size(); i != end; ++i)
            close(platform.fd(i));
    }
}

MirWaitHandle* MirConnection::create_surface(
    MirSurfaceParameters const & params,
    mir_surface_callback callback,
    void * context)
{
    auto surface = new MirSurface(this, server, platform->create_buffer_factory(), input_platform, params, callback, context);

    return surface->get_create_wait_handle();
}

char const * MirConnection::get_error_message()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (connect_result.has_error())
    {
        return connect_result.error().c_str();
    }

    return error_message.c_str();
}

void MirConnection::set_error_message(std::string const& error)
{
    error_message = error;
}


/* struct exists to work around google protobuf being able to bind
 "only 0, 1, or 2 arguments in the NewCallback function */
struct MirConnection::SurfaceRelease
{
    MirSurface * surface;
    MirWaitHandle * handle;
    mir_surface_callback callback;
    void * context;
};

void MirConnection::released(SurfaceRelease data)
{
    surface_map->erase(data.surface->id());

    // Erasing this surface from surface_map means that it will no longer receive events
    // If it's still focused, send an unfocused event before we kill it entirely
    if (data.surface->attrib(mir_surface_attrib_focus) == mir_surface_focused)
    {
        MirEvent unfocus;
        unfocus.type = mir_event_type_surface;
        unfocus.surface.id = data.surface->id();
        unfocus.surface.attrib = mir_surface_attrib_focus;
        unfocus.surface.value = mir_surface_unfocused;
        data.surface->handle_event(unfocus);
    }
    data.callback(data.surface, data.context);
    data.handle->result_received();
    delete data.surface;
}

MirWaitHandle* MirConnection::release_surface(
        MirSurface *surface,
        mir_surface_callback callback,
        void * context)
{
    auto new_wait_handle = new MirWaitHandle;

    SurfaceRelease surf_release{surface, new_wait_handle, callback, context};

    mir::protobuf::SurfaceId message;
    message.set_value(surface->id());

    {
        std::lock_guard<decltype(release_wait_handle_guard)> rel_lock(release_wait_handle_guard);
        release_wait_handles.push_back(new_wait_handle);
    }

    server.release_surface(0, &message, &void_response,
                           gp::NewCallback(this, &MirConnection::released, surf_release));


    return new_wait_handle;
}

namespace
{
void default_lifecycle_event_handler(MirLifecycleState transition)
{
    if (transition == mir_lifecycle_connection_lost)
    {
        raise(SIGTERM);
    }
}
}


void MirConnection::connected(mir_connected_callback callback, void * context)
{
    bool safe_to_callback = true;
    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        if (!connect_result.has_platform() || !connect_result.has_display_configuration())
        {
            if (!connect_result.has_error())
            {
                // We're handling an error scenario that means we're not sync'd
                // with the client code - a callback isn't safe (or needed)
                safe_to_callback = false;
                set_error_message("Connect failed");
            }
        }
        /*
         * We need to create the client platform after the connection has been
         * established, to ensure that the client platform has access to all
         * needed data (e.g. platform package).
         */
        platform = client_platform_factory->create_client_platform(this);
        native_display = platform->create_egl_native_display();
        display_configuration->set_configuration(connect_result.display_configuration());
        lifecycle_control->set_lifecycle_event_handler(default_lifecycle_event_handler);
    }

    if (safe_to_callback) callback(this, context);
    connect_wait_handle.result_received();
}

MirWaitHandle* MirConnection::connect(
    const char* app_name,
    mir_connected_callback callback,
    void * context)
{
    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        connect_parameters.set_application_name(app_name);
        connect_wait_handle.expect_result();
    }

    server.connect(
        0,
        &connect_parameters,
        &connect_result,
        google::protobuf::NewCallback(
            this, &MirConnection::connected, callback, context));
    return &connect_wait_handle;
}

void MirConnection::done_disconnect()
{
    /* todo: keeping all MirWaitHandles from a release surface until the end of the connection
       is a kludge until we have a better story about the lifetime of MirWaitHandles */
    {
        std::lock_guard<decltype(release_wait_handle_guard)> lock(release_wait_handle_guard);
        for (auto handle : release_wait_handles)
            delete handle;
    }

    disconnect_wait_handle.result_received();
}

MirWaitHandle* MirConnection::disconnect()
{
    server.disconnect(0, &ignored, &ignored,
                      google::protobuf::NewCallback(this, &MirConnection::done_disconnect));

    return &disconnect_wait_handle;
}

void MirConnection::done_drm_auth_magic(mir_drm_auth_magic_callback callback,
                                        void* context)
{
    int const status_code{drm_auth_magic_status.status_code()};

    callback(status_code, context);
    drm_auth_magic_wait_handle.result_received();
}

MirWaitHandle* MirConnection::drm_auth_magic(unsigned int magic,
                                             mir_drm_auth_magic_callback callback,
                                             void* context)
{
    mir::protobuf::DRMMagic request;
    request.set_magic(magic);

    server.drm_auth_magic(
        0,
        &request,
        &drm_auth_magic_status,
        google::protobuf::NewCallback(this, &MirConnection::done_drm_auth_magic,
                                      callback, context));

    return &drm_auth_magic_wait_handle;
}

bool MirConnection::is_valid(MirConnection *connection)
{
    {
        std::lock_guard<std::mutex> lock(connection_guard);
        if (valid_connections.count(connection) == 0)
           return false;
    }

    std::lock_guard<decltype(connection->mutex)> lock(connection->mutex);
    return !connection->connect_result.has_error();
}

void MirConnection::populate(MirPlatformPackage& platform_package)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    if (!connect_result.has_error() && connect_result.has_platform())
    {
        auto const& platform = connect_result.platform();

        platform_package.data_items = platform.data_size();
        for (int i = 0; i != platform.data_size(); ++i)
            platform_package.data[i] = platform.data(i);

        platform_package.fd_items = platform.fd_size();
        for (int i = 0; i != platform.fd_size(); ++i)
            platform_package.fd[i] = platform.fd(i);

        for (auto d : extra_platform_data)
            platform_package.data[platform_package.data_items++] = d;
    }
    else
    {
        platform_package.data_items = 0;
        platform_package.fd_items = 0;
    }
}

MirDisplayConfiguration* MirConnection::create_copy_of_display_config()
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return display_configuration->copy_to_client();
}

void MirConnection::available_surface_formats(
    MirPixelFormat* formats,
    unsigned int formats_size,
    unsigned int& valid_formats)
{
    valid_formats = 0;

    std::lock_guard<decltype(mutex)> lock(mutex);
    if (!connect_result.has_error())
    {
        valid_formats = std::min(
            static_cast<unsigned int>(connect_result.surface_pixel_format_size()),
            formats_size);

        for (auto i = 0u; i < valid_formats; i++)
        {
            formats[i] = static_cast<MirPixelFormat>(connect_result.surface_pixel_format(i));
        }
    }
}

std::shared_ptr<mir::client::ClientPlatform> MirConnection::get_client_platform()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return platform;
}

MirConnection* MirConnection::mir_connection()
{
    return this;
}

EGLNativeDisplayType MirConnection::egl_native_display()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    return *native_display;
}

void MirConnection::on_surface_created(int id, MirSurface* surface)
{
    surface_map->insert(id, surface);
}

void MirConnection::register_lifecycle_event_callback(mir_lifecycle_event_callback callback, void* context)
{
    lifecycle_control->set_lifecycle_event_handler(std::bind(callback, this, std::placeholders::_1, context));
}

void MirConnection::register_display_change_callback(mir_display_config_callback callback, void* context)
{
    display_configuration->set_display_change_handler(std::bind(callback, this, context));
}

bool MirConnection::validate_user_display_config(MirDisplayConfiguration* config)
{
    MirDisplayConfigurationStore orig_config{display_configuration->copy_to_client()};

    if ((!config) || (config->num_outputs == 0) || (config->outputs == NULL) ||
        (config->num_outputs > orig_config->num_outputs))
    {
        return false;
    }

    for(auto i = 0u; i < config->num_outputs; i++)
    {
        auto const& output = config->outputs[i];
        auto const& orig_output = orig_config->outputs[i];

        if (output.output_id != orig_output.output_id)
            return false;

        if (output.connected && output.current_mode >= orig_output.num_modes)
            return false;
    }

    return true;
}

void MirConnection::done_display_configure()
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    set_error_message(display_configuration_response.error());

    if (!display_configuration_response.has_error())
        display_configuration->set_configuration(display_configuration_response);

    return configure_display_wait_handle.result_received();
}

MirWaitHandle* MirConnection::configure_display(MirDisplayConfiguration* config)
{
    if (!validate_user_display_config(config))
    {
        return NULL;
    }

    mir::protobuf::DisplayConfiguration request;
    {
        std::lock_guard<decltype(mutex)> lock(mutex);

        for (auto i=0u; i < config->num_outputs; i++)
        {
            auto output = config->outputs[i];
            auto display_request = request.add_display_output();
            display_request->set_output_id(output.output_id);
            display_request->set_used(output.used);
            display_request->set_current_mode(output.current_mode);
            display_request->set_position_x(output.position_x);
            display_request->set_position_y(output.position_y);
            display_request->set_power_mode(output.power_mode);
        }
    }

    server.configure_display(0, &request, &display_configuration_response,
        google::protobuf::NewCallback(this, &MirConnection::done_display_configure));

    return &configure_display_wait_handle;
}

bool MirConnection::set_extra_platform_data(
    std::vector<int> const& extra_platform_data_arg)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    auto const total_data_size =
        connect_result.platform().data_size() + extra_platform_data_arg.size();

    if (total_data_size > mir_platform_package_max)
        return false;

    extra_platform_data = extra_platform_data_arg;
    return true;
}
