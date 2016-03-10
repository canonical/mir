/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/client_buffer_factory.h"
#include "mir/client_buffer.h"
#include "rpc/mir_display_server.h"
#include "presentation_chain.h"
#include "protobuf_to_native_buffer.h"
#include "buffer_factory.h"
#include <boost/throw_exception.hpp>
#include <algorithm>

namespace mcl = mir::client;
namespace geom = mir::geometry;
namespace mp = mir::protobuf;
namespace gp = google::protobuf;

mcl::PresentationChain::PresentationChain(
    MirConnection* connection,
    int stream_id,
    mir::client::rpc::DisplayServer& server,
    std::shared_ptr<mcl::ClientBufferFactory> const& native_buffer_factory,
    std::shared_ptr<mcl::AsyncBufferFactory> const& mir_buffer_factory) :
    connection_(connection),
    stream_id(stream_id),
    server(server),
    native_buffer_factory(native_buffer_factory),
    mir_buffer_factory(mir_buffer_factory)
{
}

//note: the caller of the mir::client::rpc::DisplayServer interface
//      has to provide the object for the server to fill when we get a
//      response, but we can't pass ownership of the response object to
//      the server given the current interface, nor do we know when the
//      server has been destroyed, so we have to allow for responses
//      to complete after the PresentationChain object has been deleted.
//      (mcl::BufferStream has a similar situation)
static void ignore_response(mp::Void* response)
{
    delete response;
}

void mcl::PresentationChain::allocate_buffer(
    geom::Size, MirPixelFormat, MirBufferUsage,
    mir_buffer_callback, void*)
{
    BOOST_THROW_EXCEPTION(std::logic_error("remove this soon"));
}

void mcl::PresentationChain::submit_buffer(MirBuffer* buffer)
{
    mp::BufferRequest request;
    {
        auto b = reinterpret_cast<mcl::Buffer*>(buffer);
        request.mutable_id()->set_value(stream_id);
        request.mutable_buffer()->set_buffer_id(b->rpc_id());
        b->submitted();
    }

    auto ignored = new mp::Void;
    server.submit_buffer(&request, ignored, gp::NewCallback(ignore_response, ignored));
}

void mcl::PresentationChain::release_buffer(MirBuffer*)
{
    BOOST_THROW_EXCEPTION(std::logic_error("remove this soon"));
}

void mcl::PresentationChain::buffer_available(mp::Buffer const&)
{
    BOOST_THROW_EXCEPTION(std::logic_error("remove this soon"));
}

void mcl::PresentationChain::buffer_unavailable()
{
}

int mcl::PresentationChain::rpc_id() const
{
    return stream_id;
}

MirConnection* mcl::PresentationChain::connection() const
{
    return connection_;
}

char const* mcl::PresentationChain::error_msg() const
{
    return "";
}
