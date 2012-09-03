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

namespace
{
class MirClientConnection;
class MirClientSurface;
}

struct MirSurface
{
    MirClientConnection * client_connection;
    MirClientSurface * client_surface;
};

namespace
{

namespace mc = mir::client;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;

class MirClientSurface
{
public:
    MirClientSurface(mir_surface_created_callback callback, void * context)
        : callback(callback), context(context)
    {
    }

    void run_callback(MirSurface * surface)
    {
        callback(surface, context);
    }
    mp::Surface& surface()
    {
        return surface_;
    }
    int id() const
    {
        return surface_.id().value();
    }
    char const * get_error_message()
    {
        return error_message.c_str();
    }
private:
    mir_surface_created_callback const callback;
    void * const context;
    mp::Surface surface_;
    std::string error_message;
};

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

    void create_surface(MirSurface * surface,
                        MirSurfaceParameters const & params,
                        mir_surface_created_callback callback,
                        void * context)
    {
        surface->client_connection = this;
        surface->client_surface = new MirClientSurface(callback, context);

        mir::protobuf::SurfaceParameters message;
        message.set_width(params.width);
        message.set_height(params.height);
        message.set_pixel_format(params.pixel_format);

        server.create_surface(0, &message, &surface->client_surface->surface(),
                              gp::NewCallback(this, &MirClientConnection::surface_created, surface));
    }

    void release_surface(MirSurface * surface)
    {
        mir::protobuf::SurfaceId message;
        message.set_value(surface->client_surface->id());
        server.release_surface(0, &message, &void_response,
                               gp::NewCallback(this, &MirClientConnection::surface_released, surface));
    }

    char const * get_error_message()
    {
        return error_message.c_str();
    }

    mp::Surface const & get_surface() const
    {
        return surface;
    }

private:
    void released(mir_disconnected_callback callback, void * context)
    {
        callback(context);
        delete this;
    }

    void surface_created(MirSurface * surface)
    {
        surfaces.insert(surface);
        surface->client_surface->run_callback(surface);
    }

    void surface_released(MirSurface * surface)
    {
        if (surfaces.erase(surface) == 1)
        {
            delete surface->client_surface;
            delete surface;
        }
        else
        {
            log->error() << "failed to release surface " << surface << '\n';
        }
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

void mir_connection_release(MirConnection * connection,
                            mir_disconnected_callback callback, void * context)
{
    connection->client_connection->disconnect(callback, context);
    delete connection;
}

void mir_surface_create(MirConnection * connection,
                        MirSurfaceParameters const * params,
                        mir_surface_created_callback callback,
                        void * context)
{
    MirSurface * surface = new MirSurface();
    try
    {
        connection->client_connection->create_surface(surface, *params, callback, context);
    }
    catch (std::exception const& /*x*/)
    {
        surface->client_connection = 0;
        callback(surface, context);
    }
}

void mir_surface_release(MirSurface * surface)
{
    surface->client_connection->release_surface(surface);
}

int mir_surface_is_valid(MirSurface * surface)
{
    return surface->client_surface ? 1 : 0;
}

char const * mir_surface_get_error_message(MirSurface * surface)
{
    return surface->client_surface->get_error_message();
}

MirSurfaceParameters mir_surface_get_parameters(MirSurface * surface)
{
    mp::Surface const & sf = surface->client_surface->surface();
    return MirSurfaceParameters{sf.width(), sf.height(),
                                static_cast<MirPixelFormat>(sf.pixel_format())};
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
