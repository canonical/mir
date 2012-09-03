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

#include <cstddef>

namespace
{

namespace mc = mir::client;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;

// TODO surface implementation, multiple surfaces per client

void null_callback()
{
}

class MirClient
{
public:
    MirClient(std::shared_ptr<mc::Logger> const & log)
        : channel("./mir_socket_test", log)
        , server(&channel)
    {
    }

    MirClient(MirClient const &) = delete;
    MirClient& operator=(MirClient const &) = delete;

    void disconnect(mir_disconnected_callback callback, void * context)
    {
        mir::protobuf::Void message;
        server.disconnect(0, &message, &void_response,
                          gp::NewCallback(this, &MirClient::release, callback, context));
    }

    void create_surface(MirSurface * surface_,
                        MirSurfaceParameters const & params,
                        mir_surface_created_callback callback,
                        void * context)
    {
        mir::protobuf::SurfaceParameters message;
        message.set_width(params.width);
        message.set_height(params.height);
        message.set_pixel_format(params.pixel_format);

        server.create_surface(0, &message, &surface,
                              gp::NewCallback(callback, surface_, context));
    }

    void release_surface()
    {
        mir::protobuf::Void message;
        server.release_surface(0, &message, &void_response, gp::NewCallback(null_callback));
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
    void release(mir_disconnected_callback callback, void * context)
    {
        callback(context);
        delete this;
    }
    mc::MirRpcChannel channel;
    mp::DisplayServer::Stub server;
    mp::Surface surface;
    mp::Void void_response;

    std::string error_message;
    std::mutex mutex;
};

}

struct MirConnection
{
    MirClient * client;
};

struct MirSurface
{
    MirClient * client;
};

void mir_connect(mir_connected_callback callback, void * context)
{
    MirConnection * connection = new MirConnection();

    try
    {
        auto log = std::make_shared<mc::ConsoleLogger>();
        connection->client = new MirClient(log);
    }
    catch (std::exception const& /*x*/)
    {
        connection->client = 0; // or Some error object
    }    
    callback(connection, context);
}

int mir_connection_is_valid(MirConnection * connection)
{
    return connection->client ? 1 : 0;
}

char const * mir_connection_get_error_message(MirConnection * connection)
{
    return connection->client->get_error_message();
}

void mir_connection_release(MirConnection * connection,
                            mir_disconnected_callback callback, void * context)
{
    connection->client->disconnect(callback, context);
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
        connection->client->create_surface(surface, *params, callback, context);
        surface->client = connection->client;
    }
    catch (std::exception const& /*x*/)
    {
        surface->client = 0;
        callback(surface, context);
    }
}

int mir_surface_is_valid(MirSurface * surface)
{
    return surface->client ? 1 : 0;
}

char const * mir_surface_get_error_message(MirSurface * surface)
{
    return surface->client->get_error_message();
}

MirSurfaceParameters mir_surface_get_parameters(MirSurface * surface)
{
    mp::Surface const & sf = surface->client->get_surface();
    return MirSurfaceParameters{sf.width(), sf.height(),
                                static_cast<MirPixelFormat>(sf.pixel_format())};
}

void mir_surface_release(MirSurface * surface)
{
    surface->client->release_surface();
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
