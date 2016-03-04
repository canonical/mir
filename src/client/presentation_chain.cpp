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
    std::shared_ptr<mcl::ClientBufferFactory> const& cfactory,
    std::shared_ptr<mcl::AsyncBufferFactory> const& factory) :
    connection_(connection),
    stream_id(stream_id),
    server(server),
    cfactory(cfactory),
    factory(factory)
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
    geom::Size size, MirPixelFormat format, MirBufferUsage usage,
    mir_buffer_callback cb, void* cb_context)
{
    factory->expect_buffer(cfactory, size, format, usage, cb, cb_context);

    mp::BufferAllocation request;
    request.mutable_id()->set_value(stream_id);
    auto buffer_request = request.add_buffer_requests();
    buffer_request->set_width(size.width.as_int());
    buffer_request->set_height(size.height.as_int());
    buffer_request->set_pixel_format(format);
    buffer_request->set_buffer_usage(usage);

    auto ignored = new mp::Void;
    server.allocate_buffers(&request, ignored, gp::NewCallback(ignore_response, ignored));
}

void mcl::PresentationChain::submit_buffer(MirBuffer* buffer)
{
    mp::BufferRequest request;
    {
        std::lock_guard<decltype(mutex)> lk(mutex);
        auto buffer_it = std::find_if(buffers.begin(), buffers.end(),
            [&buffer](std::unique_ptr<Buffer> const& it)
            { return reinterpret_cast<MirBuffer*>(it.get()) == buffer; });
        if (buffer_it == buffers.end())
            BOOST_THROW_EXCEPTION(std::logic_error("unknown buffer"));

        request.mutable_id()->set_value(stream_id);
        request.mutable_buffer()->set_buffer_id((*buffer_it)->rpc_id());
        (*buffer_it)->submitted();
    }

    auto ignored = new mp::Void;
    server.submit_buffer(&request, ignored, gp::NewCallback(ignore_response, ignored));
}

void mcl::PresentationChain::release_buffer(MirBuffer* buffer)
{
    mp::BufferRelease request;
    {
        std::lock_guard<decltype(mutex)> lk(mutex);
        auto buffer_it = std::find_if(buffers.begin(), buffers.end(),
            [&buffer](std::unique_ptr<Buffer> const& it)
            { return reinterpret_cast<MirBuffer*>(it.get()) == buffer; });
        if (buffer_it == buffers.end())
            BOOST_THROW_EXCEPTION(std::logic_error("unknown buffer"));

        request.mutable_id()->set_value(stream_id);
        auto buffer_req = request.add_buffers();
        buffer_req->set_buffer_id((*buffer_it)->rpc_id());

        buffers.erase(buffer_it);
    }

    auto ignored = new mp::Void;
    server.release_buffers(&request, ignored, gp::NewCallback(ignore_response, ignored));
}

void mcl::PresentationChain::buffer_available(mp::Buffer const& buffer)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    //first see if this buffer has been here before
    auto package = mcl::protobuf_to_native_buffer(buffer);
    auto buffer_it = std::find_if(buffers.begin(), buffers.end(),
        [&buffer](std::unique_ptr<Buffer> const& b)
        { return buffer.buffer_id() == b->rpc_id(); });
    if (buffer_it != buffers.end())
    {
        (*buffer_it)->received(*package);
        return;
    }

    buffers.emplace_back(factory->generate_buffer(buffer));
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



















void mcl::AsyncBufferFactory::expect_buffer(
    std::shared_ptr<mcl::ClientBufferFactory> const& factory,
    geometry::Size size,
    MirPixelFormat format,
    MirBufferUsage usage,
    mir_buffer_callback cb,
    void* cb_context)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    allocation_requests.emplace_back(
        std::make_unique<AllocationRequest>(factory, size, format, usage, cb, cb_context));
}

std::unique_ptr<mcl::Buffer> mcl::AsyncBufferFactory::generate_buffer(
    mir::protobuf::Buffer const& buffer)
{
    //must be new, allocate and send it.
    auto request_it = std::find_if(allocation_requests.begin(), allocation_requests.end(),
        [&buffer](std::unique_ptr<AllocationRequest> const& it)
        {
            return geom::Size{buffer.width(), buffer.height()} == it->size;
        });

    if (request_it == allocation_requests.end())
        BOOST_THROW_EXCEPTION(std::logic_error("unrequested buffer received"));

    auto package = mcl::protobuf_to_native_buffer(buffer);
    auto b = std::make_unique<Buffer>(
        (*request_it)->cb, (*request_it)->cb_context,
        buffer.buffer_id(),
        (*request_it)->factory->create_buffer(package, (*request_it)->size, (*request_it)->format));
    allocation_requests.erase(request_it);
    return std::move(b);
}

mcl::AsyncBufferFactory::AllocationRequest::AllocationRequest(
    std::shared_ptr<mcl::ClientBufferFactory> const& factory,
    geometry::Size size, MirPixelFormat format, MirBufferUsage usage,
    mir_buffer_callback cb, void* cb_context) :
    factory(factory),
    size(size),
    format(format),
    usage(usage),
    cb(cb),
    cb_context(cb_context)
{
}

