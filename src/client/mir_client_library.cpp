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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_client/mir_client_library.h"

#include "mir_client/mir_connection.h"
#include "mir_client/mir_surface.h"
#include "mir_client/client_platform_factory.h"

#include "mir_client/mir_rpc_channel.h"

#include "mir_protobuf.pb.h"

#include <set>
#include <unordered_set>
#include <cstddef>

namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;

#ifdef MIR_USING_BOOST_THREADS
    using ::mir::std::condition_variable;
    using ::boost::unique_lock;
    using ::boost::lock_guard;
    using ::boost::thread;
    using ::boost::mutex;
    using ::mir::std::this_thread::yield;
#else
    using namespace std;
    using std::this_thread::yield;
#endif

mutex MirConnection::connection_guard;
std::unordered_set<MirConnection *> MirConnection::valid_connections;

namespace
{
MirConnection error_connection;

class RealClientPlatformFactory : public mcl::ClientPlatformFactory
{
public:
    std::shared_ptr<mcl::ClientPlatform> create_client_platform(mcl::ClientContext* context)
    {
        return mcl::create_client_platform(context);
    }
};

}

MirWaitHandle* mir_connect(char const* socket_file, char const* name, mir_connected_callback callback, void * context)
{

    try
    {
        auto log = std::make_shared<mcl::ConsoleLogger>();
        auto client_platform_factory = std::make_shared<RealClientPlatformFactory>();
        MirConnection* connection = new MirConnection(socket_file, log, client_platform_factory);
        return connection->connect(name, callback, context);
    }
    catch (std::exception const& x)
    {
        error_connection.set_error_message(x.what());
        callback(&error_connection, context);
        return 0;
    }
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

MirWaitHandle* mir_surface_create(MirConnection * connection,
                        MirSurfaceParameters const * params,
                        mir_surface_lifecycle_callback callback,
                        void * context)
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

MirWaitHandle* mir_surface_release(MirSurface * surface,
                         mir_surface_lifecycle_callback callback, void * context)
{
    return surface->release_surface(callback, context);
}

int mir_debug_surface_id(MirSurface * surface)
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

void mir_wait_for(MirWaitHandle* wait_handle)
{
    if (wait_handle)
        wait_handle->wait_for_result();
}

MirEGLNativeWindowType mir_surface_get_egl_native_window(MirSurface *surface)
{
    return surface->generate_native_window();
}
