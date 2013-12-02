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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_CLIENT_MIR_CONNECTION_H_
#define MIR_CLIENT_MIR_CONNECTION_H_

#include <string>
#include <memory>
#include <unordered_set>
#include <unordered_map>

#include <mutex>

#include "mir_protobuf.pb.h"

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_client_library_drm.h"

#include "client_platform.h"
#include "client_context.h"

#include "mir_wait_handle.h"

namespace mir
{
/// The client-side library implementation namespace
namespace client
{
class ConnectionConfiguration;
class ClientPlatformFactory;
class SurfaceMap;
class DisplayConfiguration;
class LifecycleControl;

namespace rpc
{
class MirBasicRpcChannel;
}
}

namespace input
{
namespace receiver
{
class InputPlatform;
}
}

namespace logging
{
class Logger;
}
}

struct MirConnection : mir::client::ClientContext
{
public:
    MirConnection();

    MirConnection(mir::client::ConnectionConfiguration& conf);
    ~MirConnection() noexcept;

    MirConnection(MirConnection const &) = delete;
    MirConnection& operator=(MirConnection const &) = delete;

    MirWaitHandle* create_surface(
        MirSurfaceParameters const & params,
        mir_surface_callback callback,
        void * context);
    MirWaitHandle* release_surface(
            MirSurface *surface,
            mir_surface_callback callback,
            void *context);

    char const * get_error_message();
    void set_error_message(std::string const& error);

    MirWaitHandle* connect(
        const char* app_name,
        mir_connected_callback callback,
        void * context);

    MirWaitHandle* disconnect();

    MirWaitHandle* drm_auth_magic(unsigned int magic,
                                  mir_drm_auth_magic_callback callback,
                                  void* context);

    void register_lifecycle_event_callback(mir_lifecycle_event_callback callback, void* context);

    void register_display_change_callback(mir_display_config_callback callback, void* context);

    void populate(MirPlatformPackage& platform_package);
    MirDisplayConfiguration* create_copy_of_display_config();
    void available_surface_formats(MirPixelFormat* formats,
                                   unsigned int formats_size, unsigned int& valid_formats);

    std::shared_ptr<mir::client::ClientPlatform> get_client_platform();

    static bool is_valid(MirConnection *connection);

    MirConnection* mir_connection();

    EGLNativeDisplayType egl_native_display();

    void on_surface_created(int id, MirSurface* surface);

    MirWaitHandle* configure_display(MirDisplayConfiguration* configuration);
    void done_display_configure();

    bool set_extra_platform_data(std::vector<int> const& extra_platform_data);

private:
    std::recursive_mutex mutex; // Protects all members of *this

    std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> channel;
    mir::protobuf::DisplayServer::Stub server;
    std::shared_ptr<mir::logging::Logger> logger;
    mir::protobuf::Void void_response;
    mir::protobuf::Connection connect_result;
    mir::protobuf::Void ignored;
    mir::protobuf::ConnectParameters connect_parameters;
    mir::protobuf::DRMAuthMagicStatus drm_auth_magic_status;
    mir::protobuf::DisplayConfiguration display_configuration_response;

    std::shared_ptr<mir::client::ClientPlatformFactory> const client_platform_factory;
    std::shared_ptr<mir::client::ClientPlatform> platform;
    std::shared_ptr<EGLNativeDisplayType> native_display;

    std::shared_ptr<mir::input::receiver::InputPlatform> const input_platform;

    std::string error_message;

    MirWaitHandle connect_wait_handle;
    MirWaitHandle disconnect_wait_handle;
    MirWaitHandle drm_auth_magic_wait_handle;
    MirWaitHandle configure_display_wait_handle;

    std::mutex release_wait_handle_guard;
    std::vector<MirWaitHandle*> release_wait_handles;

    std::shared_ptr<mir::client::DisplayConfiguration> const display_configuration;

    std::shared_ptr<mir::client::LifecycleControl> const lifecycle_control;

    std::shared_ptr<mir::client::SurfaceMap> surface_map;

    std::vector<int> extra_platform_data;

    struct SurfaceRelease;

    void done_disconnect();
    void connected(mir_connected_callback callback, void * context);
    void released(SurfaceRelease );
    void done_drm_auth_magic(mir_drm_auth_magic_callback callback, void* context);
    bool validate_user_display_config(MirDisplayConfiguration* config);
};

#endif /* MIR_CLIENT_MIR_CONNECTION_H_ */
