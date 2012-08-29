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

namespace mc = mir::client;
namespace mp = mir::protobuf;

struct MirClient
{
    mc::MirRpcChannel channel;
    mp::DisplayServer::Stub server;
    mp::Surface surface;

    void connect()
    {
        mir::protobuf::ConnectMessage connect_message;
        connect_message.set_width(0);
        connect_message.set_height(0);
        connect_message.set_pixel_format(0);

        server.connect(
            0,
            &connect_message,
            &surface,
            google::protobuf::NewCallback(this, &MirClient::done_connect));
    }

    void done_connect()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            connected = true;
        }
        callback(connection, context);
    }

    bool is_valid()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return connected;
    }

    char const * get_error_message()
    {
        return error_message.c_str();
    }

    MirClient(MirConnection * connection,
              std::shared_ptr<mc::Logger> const & log,
              mir_connected_callback callback,
              void * context)
        : channel("./mir_socket_test", log)
        , server(&channel)
        , connection(connection)
        , connected(false)
        , callback(callback)
        , context(context)
    {
    }

    MirConnection * connection;
    bool connected;
    std::string error_message;
    mir_connected_callback callback;
    void * context;
    std::mutex mutex;
};

struct MirConnection
{
    MirClient * client;
};

MirConnection * create_connection(mir_connected_callback callback, void * context)
{
    MirConnection * connection = new MirConnection;
    auto log = std::make_shared<mc::ConsoleLogger>();
    MirClient * client = new MirClient(connection, log, callback, context);

    connection->client = client;
    client->connect();
    return connection;
}

void mir_connect(mir_connected_callback callback, void * context)
{
    create_connection(callback, context);
}

int mir_connection_is_valid(MirConnection * connection)
{
    return connection->client->is_valid() ? 1 : 0;
}

char const * mir_connection_get_error_message(MirConnection * connection)
{
    return connection->client->get_error_message();
}

void mir_create_surface(MirConnection *,
                        MirSurfaceParameters const *,
                        mir_surface_created_callback callback,
                        void * context)
{
    callback(NULL, context);
}

int mir_surface_is_valid(MirSurface *)
{
    return 0;
}

char const * mir_surface_get_error_message(MirSurface *)
{
    return "not yet implemented!";
}

MirSurfaceParameters mir_surface_get_parameters(MirSurface *)
{
    return MirSurfaceParameters{0, 0, mir_pixel_format_rgba_8888};
}

void mir_surface_release(MirSurface *)
{
}

void mir_advance_buffer(MirSurface *,
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
