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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_logger.h"

#include "mir_connection.h"
#include "mir_surface.h"
#include "client_platform.h"
#include "client_platform_factory.h"
#include "client_buffer_depository.h"
#include "make_rpc_channel.h"

#include "input/input_platform.h"

#include <thread>
#include <cstddef>

namespace mcl = mir::client;
namespace mcli = mcl::input;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;

MirConnection::MirConnection() :
    channel(),
    server(0),
    error_message("ERROR")
{
}

MirConnection::MirConnection(
    std::shared_ptr<google::protobuf::RpcChannel> const& channel,
    std::shared_ptr<mcl::Logger> const & log,
    std::shared_ptr<mcl::ClientPlatformFactory> const& client_platform_factory) :
        channel(channel),
        server(channel.get(), ::google::protobuf::Service::STUB_DOESNT_OWN_CHANNEL),
        log(log),
        client_platform_factory(client_platform_factory),
        input_platform(mcli::InputPlatform::create())
{
    {
        std::lock_guard<std::mutex> lock(connection_guard);
        valid_connections.insert(this);
    }
    connect_result.set_error("connect not called");
}

MirConnection::~MirConnection()
{
    std::lock_guard<std::mutex> lock(connection_guard);
    valid_connections.erase(this);
}

MirWaitHandle* MirConnection::create_surface(
    MirSurfaceParameters const & params,
    MirEventDelegate const* delegate,
    mir_surface_lifecycle_callback callback,
    void * context)
{
    auto null_log = std::make_shared<mir::client::NullLogger>();
    auto surface = new MirSurface(this, server, null_log, platform->create_buffer_factory(), input_platform, params, delegate, callback, context);

    return surface->get_create_wait_handle();
}

char const * MirConnection::get_error_message()
{
    if (connect_result.has_error())
    {
        return connect_result.error().c_str();
    }
    else
    {
    return error_message.c_str();
    }
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
    mir_surface_lifecycle_callback callback;
    void * context;
};

void MirConnection::released(SurfaceRelease data)
{
    data.callback(data.surface, data.context);
    data.handle->result_received();
    delete data.surface;
}

MirWaitHandle* MirConnection::release_surface(
        MirSurface *surface,
        mir_surface_lifecycle_callback callback,
        void * context)
{
    auto new_wait_handle = new MirWaitHandle;

    SurfaceRelease surf_release{surface, new_wait_handle, callback, context};

    mir::protobuf::SurfaceId message;
    message.set_value(surface->id());
    server.release_surface(0, &message, &void_response,
                    gp::NewCallback(this, &MirConnection::released, surf_release));

    std::lock_guard<std::mutex> lock(release_wait_handle_guard);
    release_wait_handles.push_back(new_wait_handle);
    return new_wait_handle;
}

void MirConnection::connected(mir_connected_callback callback, void * context)
{
    /*
     * We need to create the client platform after the connection has been
     * established, to ensure that the client platform has access to all
     * needed data (e.g. platform package).
     */
    platform = client_platform_factory->create_client_platform(this);
    native_display = platform->create_egl_native_display();

    callback(this, context);
    connect_wait_handle.result_received();
}

MirWaitHandle* MirConnection::connect(
    const char* app_name,
    mir_connected_callback callback,
    void * context)
{
    connect_parameters.set_application_name(app_name);
    server.connect(
        0,
        &connect_parameters,
        &connect_result,
        google::protobuf::NewCallback(
            this, &MirConnection::connected, callback, context));
    return &connect_wait_handle;
}

MirWaitHandle* MirConnection::connect(
    int lightdm_id,
    const char* app_name,
    mir_connected_callback callback,
    void * context)
{
    connect_parameters.set_application_name(app_name);
    connect_parameters.set_lightdm_id(lightdm_id);
    server.connect(
        0,
        &connect_parameters,
        &connect_result,
        google::protobuf::NewCallback(
            this, &MirConnection::connected, callback, context));
    return &connect_wait_handle;
}

void MirConnection::select_focus_by_lightdm_id(int lightdm_id)
{
    mir::protobuf::LightdmId id;
    id.set_value(lightdm_id);
    server.select_focus_by_lightdm_id(0, &id, &ignored,
        google::protobuf::NewCallback(google::protobuf::DoNothing));
}


void MirConnection::done_disconnect()
{
    /* todo: keeping all MirWaitHandles from a release surface until the end of the connection
       is a kludge until we have a better story about the lifetime of MirWaitHandles */
    {
        std::lock_guard<std::mutex> lock(release_wait_handle_guard);
        for (auto handle : release_wait_handles)
            delete handle;
    }

    disconnect_wait_handle.result_received();
}

MirWaitHandle* MirConnection::disconnect()
{
    server.disconnect(
        0,
        &ignored,
        &ignored,
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

    return !connection->connect_result.has_error();
}

void MirConnection::populate(MirPlatformPackage& platform_package)
{
    if (!connect_result.has_error() && connect_result.has_platform())
    {
        auto const& platform = connect_result.platform();

        platform_package.data_items = platform.data_size();
        for (int i = 0; i != platform.data_size(); ++i)
            platform_package.data[i] = platform.data(i);

        platform_package.fd_items = platform.fd_size();
        for (int i = 0; i != platform.fd_size(); ++i)
            platform_package.fd[i] = platform.fd(i);
    }
    else
    {
        platform_package.data_items = 0;
        platform_package.fd_items = 0;
    }
}

void MirConnection::populate(MirDisplayInfo& display_info)
{
    if (!connect_result.has_error() && connect_result.has_display_info())
    {
        auto const& connection_display_info = connect_result.display_info();

        display_info.width = connection_display_info.width();
        display_info.height = connection_display_info.height();

        auto const pf_size = connection_display_info.supported_pixel_format_size();

        /* Ensure we don't overflow the supported_pixel_format array */
        display_info.supported_pixel_format_items = pf_size > mir_supported_pixel_format_max ?
                                                    mir_supported_pixel_format_max :
                                                    pf_size;

        for (int i = 0; i < display_info.supported_pixel_format_items; ++i)
        {
            display_info.supported_pixel_format[i] =
                static_cast<MirPixelFormat>(connection_display_info.supported_pixel_format(i));
        }
    }
    else
    {
        display_info.width = 0;
        display_info.height = 0;
        display_info.supported_pixel_format_items = 0;
    }
}


std::shared_ptr<mir::client::ClientPlatform> MirConnection::get_client_platform()
{
    return platform;
}

MirConnection* MirConnection::mir_connection()
{
    return this;
}

EGLNativeDisplayType MirConnection::egl_native_display()
{
    return *native_display;
}
