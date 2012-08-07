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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_SURFACE_H_
#define MIR_SURFACE_H_

// TODO move out rpc_channel/protobuf stuff with the Surface implementation
#include "mir_client/mir_rpc_channel.h"
#include "mir_protobuf.pb.h"

namespace mir
{
namespace client
{
namespace detail
{

struct SurfaceState
{
    int width;
    int height;
    int pix_format;

    virtual bool is_valid() const = 0;
    virtual void disconnect() = 0;
    virtual ~SurfaceState() {}
protected:
    SurfaceState() = default;
    SurfaceState(SurfaceState const&) = delete;
    SurfaceState& operator=(SurfaceState const&) = delete;
};
struct InvalidSurfaceState : public SurfaceState
{
    void disconnect()
    {
    }

    bool is_valid() const
    {
        return false;
    }
};
struct ValidSurfaceState : public SurfaceState
{
    mir::client::MirRpcChannel channel;
    mir::protobuf::DisplayServer::Stub display_server;

    ValidSurfaceState(int width, int height, int pix_format) :
        channel(mir::test_socket_file()),
        display_server(&channel)
    {
        GOOGLE_PROTOBUF_VERIFY_VERSION;

        mir::protobuf::ConnectMessage connect_message;
        connect_message.set_width(width);
        connect_message.set_height(height);
        connect_message.set_pixel_format(pix_format);

        mir::protobuf::Surface surface;

        display_server.connect(
            0,
            &connect_message,
            &surface,
            google::protobuf::NewCallback(&mir::client::done));

        SurfaceState::width = surface.width();
        SurfaceState::height = surface.height();
        SurfaceState::pix_format = surface.pixel_format();
    }

    void disconnect()
    {
        mir::protobuf::Void ignored;

        display_server.disconnect(
            0,
            &ignored,
            &ignored,
            google::protobuf::NewCallback(&mir::client::done));

        google::protobuf::ShutdownProtobufLibrary();
    }

    bool is_valid() const
    {
        return true;
    }
};
}

class Surface
{
public:
    Surface() : body(new detail::InvalidSurfaceState()) {}
    Surface(int width, int height, int pix_format)
        : body(new detail::ValidSurfaceState(width, height, pix_format))
    {
    }

    ~Surface()
    {
        if (body)
        {
            body->disconnect();
        }
        delete body;
    }

    bool is_valid() const
    {
        return body->is_valid();
    }

    Surface(Surface&& that) : body(that.body)
    {
        that.body = 0;
    }

    Surface& operator=(Surface&& that)
    {
        delete body;
        body = that.body;
        that.body = 0;
        return *this;
    }

private:
    detail::SurfaceState* body;
    Surface(Surface const&) = delete;
    Surface& operator=(Surface const&) = delete;
};
}
}


#endif /* MIR_SURFACE_H_ */
