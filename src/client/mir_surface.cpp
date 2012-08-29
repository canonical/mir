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

#include "mir_client/mir_surface.h"

#include "mir_client/mir_rpc_channel.h"
#include "mir_protobuf.pb.h"

#include "mir/thread/all.h"

namespace
{
// Too clever? The idea is to ensure protbuf version is verified once (on
// the first google_protobuf_guard() call) and memory is released on exit.
struct google_protobuf_guard_t
{
    google_protobuf_guard_t() { GOOGLE_PROTOBUF_VERIFY_VERSION; }
    ~google_protobuf_guard_t() { google::protobuf::ShutdownProtobufLibrary(); }
};

void google_protobuf_guard()
{
    static google_protobuf_guard_t guard;
}
}

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
    int pixel_format;

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
    mir::protobuf::Surface surface;

    std::mutex mutex;
    std::condition_variable cv;
    bool created;

    ValidSurfaceState(std::string const& socketfile, int width, int height, int pix_format, std::shared_ptr<Logger> const& log) :
        channel(socketfile, log),
        display_server(&channel),
        created(false)
    {
        google_protobuf_guard();

        mir::protobuf::ConnectMessage connect_message;
        connect_message.set_width(width);
        connect_message.set_height(height);
        connect_message.set_pixel_format(pix_format);

        display_server.connect(
            0,
            &connect_message,
            &surface,
            google::protobuf::NewCallback(this, &ValidSurfaceState::done_connect));

        std::unique_lock<std::mutex> lock(mutex);
        while (!created) cv.wait(lock);
    }

    void done_connect()
    {
        std::unique_lock<std::mutex> lock(mutex);
        SurfaceState::width = surface.width();
        SurfaceState::height = surface.height();
        SurfaceState::pixel_format = surface.pixel_format();
        created = true;
        cv.notify_one();
    }

    void disconnect()
    {
        mir::protobuf::Void ignored;

        display_server.disconnect(
            0,
            &ignored,
            &ignored,
            google::protobuf::NewCallback(this, &ValidSurfaceState::done_disconnect));

        std::unique_lock<std::mutex> lock(mutex);
        while (created) cv.wait(lock);
    }

    void done_disconnect()
    {
        std::unique_lock<std::mutex> lock(mutex);
        created = false;
        cv.notify_one();
    }

    bool is_valid() const
    {
        return created;
    }
};
}
}
}

namespace c = ::mir::client;


c::Surface::Surface() :
    body(new detail::InvalidSurfaceState())
{
}

c::Surface::Surface(
    std::string const& socketfile,
    int width,
    int height,
    int pix_format,
    std::shared_ptr<Logger> const& log) :
    body(new detail::ValidSurfaceState(socketfile, width, height, pix_format, log))
{
}

c::Surface::~Surface()
{
    if (body)
    {
        body->disconnect();
    }
    delete body;
}

bool c::Surface::is_valid() const
{
    return body->is_valid();
}

int c::Surface::width() const
{
    return body->width;
}

int c::Surface::height() const
{
    return body->height;
}

int c::Surface::pixel_format() const
{
    return body->pixel_format;
}


c::Surface::Surface(Surface&& that) : body(that.body)
{
    that.body = 0;
}

c::Surface& c::Surface::operator=(Surface&& that)
{
    delete body;
    body = that.body;
    that.body = 0;
    return *this;
}
