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

#include "mir_protobuf.pb.h"

#include <set>
#include <cstddef>


namespace mc = mir::client;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;

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
        message.set_width(params.width);
        message.set_height(params.height);
        message.set_pixel_format(params.pixel_format);

        server.create_surface(0, &message, &surface, gp::NewCallback(callback, this, context));
    }
    void release(mir_surface_lifecycle_callback callback, void * context)
    {
        mir::protobuf::SurfaceId message;
        message.set_value(surface.id().value());
        server.release_surface(0, &message, &void_response,
                               gp::NewCallback(this, &MirSurface::released, callback, context));
    }

    MirSurfaceParameters get_parameters() const
    {
        return MirSurfaceParameters {surface.width(), surface.height(),
                                     static_cast<MirPixelFormat>(surface.pixel_format())
                                    };
    }

    char const * get_error_message()
    {
        return error_message.c_str();
    }
private:

    void released(mir_surface_lifecycle_callback callback, void * context)
    {
        callback(this, context);
        delete this;
    }

    mp::DisplayServer::Stub & server;
    mp::Void void_response;
    mp::Surface surface;
    std::string error_message;
};

namespace
{


// TODO the connection should track all associated surfaces, and release them on
// disconnection.
class MirClientConnection
{
public:
    MirClientConnection(std::shared_ptr<mc::Logger> const & log)
        : channel("./mir_socket_test", log)
        , server(&channel)
        , log(log)
    {
    }

    MirClientConnection(MirClientConnection const &) = delete;
    MirClientConnection& operator=(MirClientConnection const &) = delete;

    void disconnect(mir_disconnected_callback callback, void * context)
    {
        mir::protobuf::Void message;
        server.disconnect(0, &message, &void_response,
                          gp::NewCallback(this, &MirClientConnection::released, callback, context));
    }

    MirSurface* create_surface(
        MirSurfaceParameters const & params,
        mir_surface_lifecycle_callback callback,
        void * context)
    {
        return new MirSurface(server, params, callback, context);
    }

    char const * get_error_message()
    {
        return error_message.c_str();
    }

private:
    void released(mir_disconnected_callback callback, void * context)
    {
        callback(context);
        delete this;
    }

    mc::MirRpcChannel channel;
    mp::DisplayServer::Stub server;
    std::shared_ptr<mc::Logger> log;
    mp::Surface surface;
    mp::Void void_response;

    std::string error_message;
    std::set<MirSurface *> surfaces;
};

}

struct MirConnection
{
    MirClientConnection * client_connection;
};

void mir_connect(mir_connected_callback callback, void * context)
{
    MirConnection * connection = new MirConnection();

    try
    {
        auto log = std::make_shared<mc::ConsoleLogger>();
        connection->client_connection = new MirClientConnection(log);
    }
    catch (std::exception const& /*x*/)
    {
        connection->client_connection = 0; // or Some error object
    }
    callback(connection, context);
}

int mir_connection_is_valid(MirConnection * connection)
{
    return connection->client_connection ? 1 : 0;
}

char const * mir_connection_get_error_message(MirConnection * connection)
{
    return connection->client_connection->get_error_message();
}

void mir_connection_release(MirConnection * connection)
{
    delete connection->client_connection;
    delete connection;
}

void mir_surface_create(MirConnection * connection,
                        MirSurfaceParameters const * params,
                        mir_surface_lifecycle_callback callback,
                        void * context)
{
    try
    {
        connection->client_connection->create_surface(*params, callback, context);
    }
    catch (std::exception const&)
    {
        // TODO callback with an error surface
    }
}

void mir_surface_release(MirSurface * surface,
                         mir_surface_lifecycle_callback callback, void * context)
{
    surface->release(callback, context);
}

int mir_surface_is_valid(MirSurface * /*surface*/)
{
    // TODO surfaces in an error state
    return 1;
}

char const * mir_surface_get_error_message(MirSurface * surface)
{
    return surface->get_error_message();
}

MirSurfaceParameters mir_surface_get_parameters(MirSurface * surface)
{
    return surface->get_parameters();
}

void mir_surface_advance_buffer(MirSurface *,
                                mir_buffer_advanced_callback callback,
                                void * context)
{
    callback(NULL, context);
}

int mir_buffer_is_valid(MirBuffer *)
{
    return 0;
}

char const * mir_buffer_get_error_message(MirBuffer *)
{
    return "not yet implemented!";
}

int mir_buffer_get_next_vblank_microseconds(MirBuffer *)
{
    return -1;
}
