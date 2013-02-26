/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_CLIENT_MIR_SURFACE_H_
#define MIR_CLIENT_MIR_SURFACE_H_

#include "mir_protobuf.pb.h"

#include "mir/geometry/pixel_format.h"
#include "mir/geometry/dimensions.h"
#include "mir_client/mir_client_library.h"
#include "client_buffer_depository.h"
#include "mir_wait_handle.h"
#include "mir_client_surface.h"
#include "client_platform.h"

#include <memory>

namespace mir
{
namespace client
{
class ClientBuffer;
class MemoryRegion;
}
}

class mir_client::MirSurface : public mir::client::ClientSurface
{
public:
    MirSurface(MirSurface const &) = delete;
    MirSurface& operator=(MirSurface const &) = delete;

    MirSurface(
        mir_client::MirConnection *allocating_connection,
        mir::protobuf::DisplayServer::Stub & server,
        const std::shared_ptr<mir::client::Logger>& logger,
        const std::shared_ptr<mir::client::ClientBufferDepository>& depository,
        mir_client::MirSurfaceParameters const & params,
        mir_client::mir_surface_lifecycle_callback callback, void * context);

    ~MirSurface();

    mir_client::MirWaitHandle* release_surface(
        mir_client::mir_surface_lifecycle_callback callback,
        void *context);

    mir_client::MirSurfaceParameters get_parameters() const;
    char const * get_error_message();
    int id() const;
    bool is_valid() const;
    mir_client::MirWaitHandle* next_buffer(mir_client::mir_surface_lifecycle_callback callback, void * context);
    mir_client::MirWaitHandle* get_create_wait_handle();

    std::shared_ptr<mir_client::MirBufferPackage> get_current_buffer_package();
    std::shared_ptr<mir::client::ClientBuffer> get_current_buffer();
    void get_cpu_region(mir_client::MirGraphicsRegion& region);
    void release_cpu_region();
    EGLNativeWindowType generate_native_window();

private:
    void process_incoming_buffer();
    void populate(mir_client::MirBufferPackage& buffer_package);
    void created(mir_client::mir_surface_lifecycle_callback callback, void * context);
    void new_buffer(mir_client::mir_surface_lifecycle_callback callback, void * context);
    mir::geometry::PixelFormat convert_ipc_pf_to_geometry(google::protobuf::int32 pf );

    /* todo: race condition. protobuf does not guarantee that callbacks will be synchronized. potential
             race in surface, last_buffer_id */
    mir::protobuf::DisplayServer::Stub & server;
    mir::protobuf::Surface surface;
    std::string error_message;

    mir_client::MirConnection *connection;
    mir_client::MirWaitHandle create_wait_handle;
    mir_client::MirWaitHandle next_buffer_wait_handle;

    int last_buffer_id;

    std::shared_ptr<mir::client::MemoryRegion> secured_region;
    std::shared_ptr<mir::client::ClientBufferDepository> buffer_depository;

    std::shared_ptr<mir::client::Logger> logger;
    std::shared_ptr<EGLNativeWindowType> accelerated_window;
};

#endif /* MIR_CLIENT_PRIVATE_MIR_WAIT_HANDLE_H_ */
