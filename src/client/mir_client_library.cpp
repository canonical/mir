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
#include "mir_client/mir_rpc_channel.h"
#include "mir_client/mir_buffer_package.h" 

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
#else
    using namespace std;
#endif

class MirWaitHandle
{
public:
    MirWaitHandle() : guard(), wait_condition(), waiting_for_result(false) {}

    void result_requested()
    {
        lock_guard<mutex> lock(guard);

        waiting_for_result = true;
    }

    void result_received()
    {
        lock_guard<mutex> lock(guard);

        waiting_for_result = false;

        wait_condition.notify_all();
    }

    void wait_for_result()
    {
        unique_lock<mutex> lock(guard);
        while (waiting_for_result)
            wait_condition.wait(lock);
    }

private:
    mutex guard;
    condition_variable wait_condition;
    bool waiting_for_result;
};

class MirSurface
{
public:
    MirSurface(MirSurface const &) = delete;
    MirSurface& operator=(MirSurface const &) = delete;

    MirSurface(
        mp::DisplayServer::Stub & server,
        MirSurfaceParameters const & params,
        mir_surface_lifecycle_callback callback, void * context)
        : server(server)
    {
        mir::protobuf::SurfaceParameters message;
        message.set_surface_name(params.name ? params.name : std::string());
        message.set_width(params.width);
        message.set_height(params.height);
        message.set_pixel_format(params.pixel_format);

        create_wait_handle.result_requested();
        server.create_surface(0, &message, &surface, gp::NewCallback(this, &MirSurface::created, callback, context));
    }
    MirWaitHandle* release(mir_surface_lifecycle_callback callback, void * context)
    {
        mir::protobuf::SurfaceId message;
        message.set_value(surface.id().value());
        release_wait_handle.result_requested();
        server.release_surface(0, &message, &void_response,
                               gp::NewCallback(this, &MirSurface::released, callback, context));
        return &release_wait_handle;
    }

    MirSurfaceParameters get_parameters() const
    {
        return MirSurfaceParameters {
            0,
            surface.width(),
            surface.height(),
            static_cast<MirPixelFormat>(surface.pixel_format())};
    }

    char const * get_error_message()
    {
        if (surface.has_error())
        {
            return surface.error().c_str();
        }
        return error_message.c_str();
    }

    int id() const
    {
        return surface.id().value();
    }

    bool is_valid() const
    {
        return !surface.has_error();
    }

    void populate(MirGraphicsRegion& )
    {
        // TODO
    }

    MirWaitHandle* next_buffer(mir_surface_lifecycle_callback callback, void * context)
    {
        next_buffer_wait_handle.result_requested();
        server.next_buffer(
            0,
            &surface.id(),
            surface.mutable_buffer(),
            google::protobuf::NewCallback(this, &MirSurface::new_buffer, callback, context));

        return &next_buffer_wait_handle;
    }

    MirWaitHandle* get_create_wait_handle()
    {
        return &create_wait_handle;
    }

private:

    void released(mir_surface_lifecycle_callback callback, void * context)
    {
        release_wait_handle.result_received();
        callback(this, context);
        delete this;
    }

    void created(mir_surface_lifecycle_callback callback, void * context)
    {
        create_wait_handle.result_received();
        callback(this, context);
    }

    void new_buffer(mir_surface_lifecycle_callback callback, void * context)
    {
        next_buffer_wait_handle.result_received();
        callback(this, context);
    }

    mp::DisplayServer::Stub & server;
    mp::Void void_response;
    mp::Surface surface;
    std::string error_message;

    MirWaitHandle create_wait_handle;
    MirWaitHandle release_wait_handle;
    MirWaitHandle next_buffer_wait_handle;
};

// TODO the connection should track all associated surfaces, and release them on
// disconnection.
class MirConnection
{
public:
    MirConnection(const std::string& socket_file,
        std::shared_ptr<mcl::Logger> const & log) :
          channel(socket_file, log)
        , server(&channel)
        , log(log)
    {
        {
            lock_guard<mutex> lock(connection_guard);
            valid_connections.insert(this);
        }
        connect_result.set_error("connect not called");
    }

    ~MirConnection()
    {
        {
            lock_guard<mutex> lock(connection_guard);
            valid_connections.erase(this);
        }
    }

    MirConnection(MirConnection const &) = delete;
    MirConnection& operator=(MirConnection const &) = delete;

    MirWaitHandle* create_surface(
        MirSurfaceParameters const & params,
        mir_surface_lifecycle_callback callback,
        void * context)
    {
        auto tmp = new MirSurface(server, params, callback, context);
        return tmp->get_create_wait_handle();
    }

    char const * get_error_message()
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

    MirWaitHandle* connect(
        const char* app_name,
        mir_connected_callback callback,
        void * context)
    {
        connect_parameters.set_application_name(app_name);
        connect_wait_handle.result_requested();
        server.connect(
            0,
            &connect_parameters,
            &connect_result,
            google::protobuf::NewCallback(
                this, &MirConnection::connected, callback, context));
        return &connect_wait_handle;
    }

    void disconnect()
    {
        disconnect_wait_handle.result_requested();
        server.disconnect(
            0,
            &ignored,
            &ignored,
            google::protobuf::NewCallback(this, &MirConnection::done_disconnect));

        disconnect_wait_handle.wait_for_result();
    }

    static bool is_valid(MirConnection *connection)
    {
        {
            lock_guard<mutex> lock(connection_guard);
            if (valid_connections.count(connection) == 0)
               return false;
        }

        return !connection->connect_result.has_error();
    }
private:

    mcl::MirRpcChannel channel;
    mp::DisplayServer::Stub server;
    std::shared_ptr<mcl::Logger> log;
    mp::Void void_response;
    mir::protobuf::Connection connect_result;
    mir::protobuf::Void ignored;
    mir::protobuf::ConnectParameters connect_parameters;

    std::string error_message;
    std::set<MirSurface*> surfaces;

    MirWaitHandle connect_wait_handle;
    MirWaitHandle disconnect_wait_handle;

    void done_disconnect()
    {
        disconnect_wait_handle.result_received();
    }

    void connected(mir_connected_callback callback, void * context)
    {
        connect_wait_handle.result_received();
        callback(this, context);
    }

    static mutex connection_guard;
    static std::unordered_set<MirConnection*> valid_connections;
};

mutex MirConnection::connection_guard;
std::unordered_set<MirConnection *> MirConnection::valid_connections;

MirWaitHandle* mir_connect(char const* socket_file, char const* name, mir_connected_callback callback, void * context)
{

    try
    {
        auto log = std::make_shared<mcl::ConsoleLogger>();
        MirConnection * connection = new MirConnection(socket_file, log);
        return connection->connect(name, callback, context);
    }
    catch (std::exception const& /*x*/)
    {
        // TODO callback with an error connection
        return 0; // TODO
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
    connection->disconnect();
    delete connection;
}

MirWaitHandle* mir_surface_create(MirConnection * connection,
                        MirSurfaceParameters const * params,
                        mir_surface_lifecycle_callback callback,
                        void * context)
{
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
    return surface->release(callback, context);
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

void mir_surface_get_current_buffer(MirSurface *surface, MirGraphicsRegion *buffer_package)
{
    surface->populate(*buffer_package);
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

