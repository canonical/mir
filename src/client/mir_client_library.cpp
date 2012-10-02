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

#include "mir_client/private/mir_connection.h"
#include "mir_client/private/mir_surface.h"

#include "mir_client/private/mir_rpc_channel.h"
#include "mir_client/private/mir_buffer_package.h" 

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

mutex mcl::MirConnection::connection_guard;
std::unordered_set<mcl::MirConnection *> mcl::MirConnection::valid_connections;

MirWaitHandle* mir_connect(char const* socket_file, char const* name, mir_connected_callback callback, void * context)
{

    try
    {
        auto log = std::make_shared<mcl::ConsoleLogger>();
        mcl::MirConnection * connection = new mcl::MirConnection(socket_file, log);
        return ( ::MirWaitHandle*) connection->connect(name, callback, context);
    }
    catch (std::exception const& /*x*/)
    {
        // TODO callback with an error connection
        return 0; // TODO
    }
}

int mir_connection_is_valid(MirConnection * connection)
{
    return mcl::MirConnection::is_valid((mcl::MirConnection*) connection);
}

char const * mir_connection_get_error_message(MirConnection * connection)
{
    auto connection_cast = (mcl::MirConnection*) connection;
    return connection_cast->get_error_message();
}

void mir_connection_release(MirConnection * connection)
{
    auto connection_cast = (mcl::MirConnection*) connection;
    connection_cast->disconnect();
    delete connection_cast;
}

MirWaitHandle* mir_surface_create(MirConnection * connection,
                        MirSurfaceParameters const * params,
                        mir_surface_lifecycle_callback callback,
                        void * context)
{
    try
    {
        auto connection_cast = (mcl::MirConnection*) connection;
        return (::MirWaitHandle*) connection_cast->create_surface(*params, callback, context);
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
    auto surface_cast = (mcl::MirSurface*) surface;
    return (MirWaitHandle*) surface_cast->release(callback, context);
}

int mir_debug_surface_id(MirSurface * surface)
{
    auto surface_cast = (mcl::MirSurface*) surface;
    return surface_cast->id();
}

int mir_surface_is_valid(MirSurface* surface)
{
    auto surface_cast = (mcl::MirSurface*) surface;
    return surface_cast->is_valid();
}

char const * mir_surface_get_error_message(MirSurface * surface)
{
    auto surface_cast = (mcl::MirSurface*) surface;
    return surface_cast->get_error_message();
}

void mir_surface_get_parameters(MirSurface * surface, MirSurfaceParameters *parameters)
{
    auto surface_cast = (mcl::MirSurface*) surface;
    *parameters = surface_cast->get_parameters();
}

void mir_surface_get_current_buffer(MirSurface *surface, MirGraphicsRegion *buffer_package)
{
    auto surface_cast = (mcl::MirSurface*) surface;
    surface_cast->populate(*buffer_package);
}

MirWaitHandle* mir_surface_next_buffer(MirSurface *surface, mir_surface_lifecycle_callback callback, void * context)
{
    auto surface_cast = (mcl::MirSurface*) surface;
    return (MirWaitHandle*) surface_cast->next_buffer(callback, context);
}

void mir_wait_for(MirWaitHandle* wait_handle)
{
    auto wait_handle_cast = (mcl::MirWaitHandle*) wait_handle;
    if (wait_handle_cast)
        wait_handle_cast->wait_for_result();
}
