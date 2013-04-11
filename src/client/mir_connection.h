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
class Logger;
class ClientBufferDepository;
class ClientPlatformFactory;
namespace input
{
class InputPlatform;
}
}
}

struct MirConnection : public mir::client::ClientContext
{
public:
    MirConnection();

    MirConnection(std::shared_ptr<google::protobuf::RpcChannel> const& channel,
                  std::shared_ptr<mir::client::Logger> const & log,
                  std::shared_ptr<mir::client::ClientPlatformFactory> const& client_platform_factory);
    ~MirConnection();

    MirConnection(MirConnection const &) = delete;
    MirConnection& operator=(MirConnection const &) = delete;

    MirWaitHandle* create_surface(
        MirSurfaceParameters const & params,
        mir_surface_lifecycle_callback callback,
        void * context);
    MirWaitHandle* release_surface(
            MirSurface *surface,
            mir_surface_lifecycle_callback callback,
            void *context);

    char const * get_error_message();
    void set_error_message(std::string const& error);

    MirWaitHandle* connect(
        const char* app_name,
        mir_connected_callback callback,
        void * context);

    MirWaitHandle* connect(
        int lightdm_id,
        const char* app_name,
        mir_connected_callback callback,
        void * context);

    void select_focus_by_lightdm_id(int lightdm_id);

    MirWaitHandle* disconnect();

    MirWaitHandle* drm_auth_magic(unsigned int magic,
                                  mir_drm_auth_magic_callback callback,
                                  void* context);

    void populate(MirPlatformPackage& platform_package);
    void populate(MirDisplayInfo& display_info);

    std::shared_ptr<mir::client::ClientPlatform> get_client_platform();

    static bool is_valid(MirConnection *connection);

    MirConnection* mir_connection();

    EGLNativeDisplayType egl_native_display();

private:
    std::shared_ptr<google::protobuf::RpcChannel> channel;
    mir::protobuf::DisplayServer::Stub server;
    std::shared_ptr<mir::client::Logger> log;
    mir::protobuf::Void void_response;
    mir::protobuf::Connection connect_result;
    mir::protobuf::Void ignored;
    mir::protobuf::ConnectParameters connect_parameters;
    mir::protobuf::DRMAuthMagicStatus drm_auth_magic_status;

    std::shared_ptr<mir::client::ClientPlatformFactory> const client_platform_factory;
    std::shared_ptr<mir::client::ClientPlatform> platform;
    std::shared_ptr<EGLNativeDisplayType> native_display;

    std::shared_ptr<mir::client::input::InputPlatform> const input_platform;

    std::string error_message;

    MirWaitHandle connect_wait_handle;
    MirWaitHandle disconnect_wait_handle;
    MirWaitHandle drm_auth_magic_wait_handle;

    std::mutex release_wait_handle_guard;
    std::vector<MirWaitHandle*> release_wait_handles;

    static std::mutex connection_guard;
    static std::unordered_set<MirConnection*> valid_connections;

    struct SurfaceRelease;

    void done_disconnect();
    void connected(mir_connected_callback callback, void * context);
    void released(SurfaceRelease );
    void done_drm_auth_magic(mir_drm_auth_magic_callback callback, void* context);
};

#endif /* MIR_CLIENT_MIR_CONNECTION_H_ */
